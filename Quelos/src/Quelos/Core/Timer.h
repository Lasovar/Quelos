#pragma once

#include <chrono>
#include <Quelos/Core/Ref.h>

namespace Quelos {
    class Time : public RefCounted {
    public:
        using Clock = std::chrono::steady_clock;

        void Init() {
            m_LastFrameTime = Clock::now();
            m_DeltaTime = 0.0;
            m_TimeSinceStartup = 0.0f;
        }

        void Tick() {
            const std::chrono::time_point<Clock> now = Clock::now();
            const std::chrono::duration<float> deltaTime = now - m_LastFrameTime;

            m_LastFrameTime = now;
            m_DeltaTime = std::min(deltaTime.count(), k_MaxDelta);
            m_TimeSinceStartup += m_DeltaTime;
        }

        [[nodiscard]] float DeltaTime() const { return m_DeltaTime; }

    private:
        Clock::time_point m_LastFrameTime;
        float m_DeltaTime = 0.0f;
        float m_TimeSinceStartup = 0.0f;

        static constexpr float k_MaxDelta = 0.0333f;
    };
}
