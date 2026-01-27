#pragma once
#include "bgfx/bgfx.h"

namespace Quelos {
    class Shader : public RefCounted {
    public:
        explicit Shader(const std::string& filePathVertex, const std::string& filePathFragment);
        ~Shader() override;

        bgfx::ProgramHandle GetHandle() const { return m_ProgramHandle; }
    private:
        bgfx::ProgramHandle m_ProgramHandle = BGFX_INVALID_HANDLE;
    };
}
