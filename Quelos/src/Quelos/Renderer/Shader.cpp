#include "Shader.h"

#include "Quelos/Core/Application.h"
#include "Quelos/Utility/QuelosUtil.h"

namespace Quelos {

    static bgfx::ShaderHandle LoadShader(const std::string& fileName) {
        std::string shaderPath;

        switch (bgfx::getRendererType()) {
        case bgfx::RendererType::Noop:
        case bgfx::RendererType::Direct3D11:
        case bgfx::RendererType::Direct3D12: shaderPath = "Assets/shaders/dx11/"; break;
        case bgfx::RendererType::Gnm: shaderPath = "Assets/shaders/pssl/"; break;
        case bgfx::RendererType::Metal: shaderPath = "Assets/shaders/metal/"; break;
        case bgfx::RendererType::OpenGL: shaderPath = "Assets/shaders/glsl/"; break;
        case bgfx::RendererType::OpenGLES: shaderPath = "Assets/shaders/essl/"; break;
        case bgfx::RendererType::Vulkan: shaderPath = "Assets/shaders/spirv/"; break;
        case bgfx::RendererType::Agc:
        case bgfx::RendererType::Nvn:
        case bgfx::RendererType::WebGPU:
        case bgfx::RendererType::Count:
            break;
        }

        if (const std::vector<byte> data = Utility::ReadBinaryFile(shaderPath + fileName); !data.empty()) {
            const bgfx::Memory* mem = bgfx::copy(data.data(), data.size());

            const bgfx::ShaderHandle handle = bgfx::createShader(mem);
            bgfx::setName(handle, fileName.c_str(), static_cast<int32_t>(fileName.length()));

            return handle;
        }

        return BGFX_INVALID_HANDLE;
    }

    Shader::Shader(const std::string& filePathVertex, const std::string& filePathFragment) {
        const bgfx::ShaderHandle vsh = LoadShader(filePathVertex);
        const bgfx::ShaderHandle fsh = LoadShader(filePathFragment);

        if (!bgfx::isValid(vsh)) {
            QS_CORE_ERROR("Vertex shader '{}' failed to load", filePathVertex);
            return;
        }

        if (!bgfx::isValid(fsh)) {
            QS_CORE_ERROR("Fragment shader '{}' failed to load", filePathFragment);
            return;
        }

        m_ProgramHandle = bgfx::createProgram(vsh, fsh, true);
    }

    Shader::~Shader() {
        bgfx::destroy(m_ProgramHandle);
    }
}
