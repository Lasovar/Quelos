#include "Quelos/Plugin/PluginAPI.h"
#include "QuelosEditor/EditorAPI.h"

#include "ImGui/bgfxImGuiState.h"

#include "process.hpp"
#include "Quelos/Core/Log.h"
#include "Quelos/ImGui/ImGuiUI.h"
#include "Quelos/Utility/FileSystem.h"
#include "Quelos/Utility/QuelosUtil.h"

namespace Quelos {
    std::string ToHex(const unsigned char* data, const size_t size) {
        std::string out;
        out.reserve(size * 3);

        for (size_t i = 0; i < size; ++i) {
            fmt::format_to(
                std::back_inserter(out),
                "{:02X}",
                data[i]
            );
        }

        return out;
    }

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
        std::string handle =  ToHex(desc->assetHandle.Bytes, sizeof(desc->assetHandle.Bytes));
        std::string includePath(FS::Parent(shadercPath));

        OsPath shaderBinariesPath = Project::GetLibraryPath() / "Shaders";

        std::filesystem::create_directories(shaderBinariesPath);
        std::string shaderBinariesString = shaderBinariesPath.generic_string();

        std::string vertexBinaryPath = fmt::format("{}/vs_{}.sb", shaderBinariesString, handle);
        Vec<std::string> messages;
        Vec<std::string> errors;

        Process vertex(
            {
                shadercPath,
                "-f", fmt::format("{}/vs_{}.sc", sourcePath, FS::Stem(sourcePath)),
                "-o", vertexBinaryPath,
                "--type", "vertex",
                "--profile", profileType,
                "-i", includePath,
#ifdef QS_PLATFORM_MACOS
                "--platform", "osx"
#endif
            },
            "",
            [&messages](const char* bytes, const size_t count) {
                if (!bytes || !count) {
                    return;
                }

                messages.emplace_back(bytes, count);
            },
            [&errors](const char* bytes, const size_t count) {
                if (!bytes || !count) {
                    return;
                }

                errors.emplace_back(bytes, count);
            }
        );

        const int vCode = vertex.get_exit_status();

        for (auto& message : messages) {
            QS_INFO_TAG("CompileBgfx::vertex", message);
        }

        for (auto& error : errors) {
            QS_INFO_TAG("CompileBgfx::vertex", error);
        }

        messages.clear();
        errors.clear();

        std::string fragmentBinaryPath = fmt::format("{}/fs_{}.sb", shaderBinariesString, handle);
        Process fragment(
            {
                shadercPath,
                "-f", fmt::format("{}/fs_{}.sc", sourcePath, FS::Stem(sourcePath)),
                "-o", fragmentBinaryPath,
                "--type", "fragment",
                "--profile", profileType,
                "-i", includePath,
#ifdef QS_PLATFORM_MACOS
                "--platform", "osx"
#endif
            },
            "",
            [&messages](const char* bytes, const size_t count) {
                if (!bytes || !count) {
                    return;
                }

                messages.emplace_back(bytes, count);
            },
            [&errors](const char* bytes, const size_t count) {
                if (!bytes || !count) {
                    return;
                }

                errors.emplace_back(bytes, count);
            }
        );

        const int fCode = fragment.get_exit_status();

        for (auto& message : messages) {
            QS_INFO_TAG("CompileBgfx::fragment", message);
        }

        for (auto& error : errors) {
            QS_INFO_TAG("CompileBgfx::fragment", error);
        }

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

        vertexBuffer.disown();
        fragmentBuffer.disown();

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
