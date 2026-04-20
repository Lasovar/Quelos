#include "Quelos/Core/Base.h"
#include "Quelos/Plugin/PluginAPI.h"
#include "QuelosEditor/EditorAPI.h"

#include "ImGui/bgfxImGuiState.h"

#include "process.hpp"
#include "Quelos/Core/Log.h"
#include "Quelos/ImGui/ImGuiUI.h"
#include "Quelos/Utility/FileSystem.h"
#include "Quelos/Utility/QuelosUtil.h"

namespace Quelos {
    static QS_ShaderOutputArray CompileBgfx(const QS_ShaderCompileDesc* desc) {
        using namespace TinyProcessLib;

        std::string profileType;
        switch (bgfx::getRendererType()) {
        case bgfx::RendererType::Noop:
        case bgfx::RendererType::Direct3D11:
        case bgfx::RendererType::Direct3D12: profileType = "dx11"; break;
        case bgfx::RendererType::Gnm: profileType = "pssl"; break;
        case bgfx::RendererType::Metal: profileType = "metal"; break;
        case bgfx::RendererType::OpenGL: profileType = "glsl"; break;
        case bgfx::RendererType::OpenGLES: profileType = "essl"; break;
        case bgfx::RendererType::Vulkan: profileType = "spirv"; break;
        case bgfx::RendererType::Agc:
        case bgfx::RendererType::Nvn:
        case bgfx::RendererType::WebGPU:
        case bgfx::RendererType::Count:
            break;
        }

        std::string shadercPath = (Application::Get().GetApplicationPath() / "Tools/bin/shaderc").generic_string();
        std::string sourcePath = (Project::GetProjectPath() / desc->sourcePath).generic_string();
        std::string includePath = std::string(FS::Parent(shadercPath));

        std::string vertexBinaryPath = fmt::format("{}/vs_{}.bin", sourcePath, FS::Stem(sourcePath));


        Process vertex(
            {
                shadercPath,
                "-f", fmt::format("{}/vs_{}.sc", sourcePath, FS::Stem(sourcePath)),
                "-o", vertexBinaryPath,
                "--type", "vertex",
                "--profile", profileType,
                "-i", includePath
            },
            "",
            [](const char* bytes, const size_t count) {
                QS_INFO_TAG("bgfxShaderCompiler::vertex", "{}", std::string_view(bytes, count));
            },
            [](const char* bytes, const size_t count) {
                QS_ERROR_TAG("bgfxShaderCompiler::vertex", "{}", std::string_view(bytes, count));
            }
        );

        std::string fragmentBinaryPath = fmt::format("{}/fs_{}.bin", sourcePath, FS::Stem(sourcePath));
        Process fragment(
            {
                shadercPath,
                "-f", fmt::format("{}/fs_{}.sc", sourcePath, FS::Stem(sourcePath)),
                "-o", fragmentBinaryPath,
                "--type", "fragment",
                "--profile", profileType,
                "-i", includePath
            },
            "",
            [](const char* bytes, const size_t count) {
                QS_INFO_TAG("bgfxShaderCompiler::fragment", "{}", std::string_view(bytes, count));
            },
            [](const char* bytes, const size_t count) {
                QS_ERROR_TAG("bgfxShaderCompiler::fragment", "{}", std::string_view(bytes, count));
            }
        );

        const int vCode = vertex.get_exit_status();
        const int fCode = fragment.get_exit_status();

        if (vCode || fCode) {
            return {
                .Count = 0
            };
        }

        Buffer vertexBuffer = Utility::ReadFile(vertexBinaryPath);
        Buffer fragmentBuffer = Utility::ReadFile(fragmentBinaryPath);

        QS_ShaderOutputArray array;
        array.Outputs[QS_SHADER_OUTPUT_VERTEX] = {
            .Data = vertexBuffer.data(),
            .Size = vertexBuffer.size()
        };

        array.Outputs[QS_SHADER_OUTPUT_FRAGMENT] = {
            .Data = fragmentBuffer.data(),
            .Size = fragmentBuffer.size()
        };

        array.Count = 2;

        vertexBuffer.release_ownership();
        fragmentBuffer.release_ownership();

        return array;
    }

    static void FreeBgfx(void* buf) {
        std::free(buf);
    }
}

QS_PLUGIN_EXPORT void RegisterBgfxEditorPlugin(QS_EditorAPI* editorAPI) {
    constexpr QS_ShaderCompiler compiler = {
        .Compile = Quelos::CompileBgfx,
        .FreeBuffer = Quelos::FreeBgfx,
    };

    editorAPI->RegisterShaderCompiler("bgfxRenderer", compiler);
}
