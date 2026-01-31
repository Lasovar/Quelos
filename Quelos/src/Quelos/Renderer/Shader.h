#pragma once

namespace Quelos {
    class Shader : public RefCounted {
    public:
        virtual void Submit(uint32_t view) = 0;
    public:
        static Ref<Shader> Create(const std::string& filePathVertex, const std::string& filePathFragment);
    };
}
