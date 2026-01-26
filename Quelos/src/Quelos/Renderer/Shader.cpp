#include "Shader.h"

#include "Quelos/Core/Application.h"

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

        std::string executable = Application::Get().GetApplicationSpecification().Executable;
        std::filesystem::path exeDir = std::filesystem::canonical(executable).parent_path();

        std::filesystem::path filePath = exeDir / (shaderPath + fileName);

        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            QS_CORE_ERROR("Couldn't open file '{}'", filePath.string());
            return BGFX_INVALID_HANDLE;
        }

        uintmax_t fileSize = std::filesystem::file_size(filePath);

        const bgfx::Memory* mem = bgfx::alloc(fileSize + 1);
        file.read(reinterpret_cast<char*>(mem->data), fileSize);

        mem->data[fileSize] = '\0';

        bgfx::ShaderHandle handle = bgfx::createShader(mem);
        bgfx::setName(handle, fileName.c_str(), fileName.length());

        return handle;
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
