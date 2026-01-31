#pragma once

#include "Quelos/Renderer/Shader.h"

#include "bgfx/bgfx.h"

namespace Quelos {
    class bgfxShader : public Shader {
    public:
        explicit bgfxShader(const std::string& filePathVertex, const std::string& filePathFragment);
        ~bgfxShader() override;

        void Submit(uint32_t view) override;
    private:
        bgfx::ProgramHandle m_ProgramHandle = BGFX_INVALID_HANDLE;
    };
}
