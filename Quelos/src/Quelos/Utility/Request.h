//
// Created by lasovar on 6/7/26.
//

#pragma once

namespace Quelos {
    // Basically a boolean that can only be resolved once
    // Useful for handling behavior that needs to be deferred to next frame
    struct Request {
    public:
        Request() = default;
        Request(const bool request) {
            m_Requested = request;
        }

        [[nodiscard]] bool Resolve() {
            if (m_Requested) {
                m_Requested = false;
                return true;
            }

            return false;
        }

        // Returns if Requested without clearing the value
        [[nodiscard]] bool IsRequested() const { return m_Requested; }

    private:
        bool m_Requested = false;
    };
}
