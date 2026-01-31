#include "Shader.h"

#include "Quelos/Core/Application.h"
#include "Quelos/Platform/bgfx/bgfxShader.h"

namespace Quelos {

    Ref<Shader> Shader::Create(const std::string& filePathVertex, const std::string& filePathFragment) {
        return CreateRef<bgfxShader>(filePathVertex, filePathFragment);
    }
}
