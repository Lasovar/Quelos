//
// Created by lasovar on 6/7/26.
//

#pragma once

#include "Quelos/Core/DataTypes.hpp"

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

    /// A Request with a payload
    template <typename T>
    struct PRequest {
    public:
        PRequest() = default;

        explicit PRequest(T payload) {
            m_Requested = true;
            Payload = payload;
        }

        [[nodiscard]] Option<T> Resolve() {
            if (m_Requested) {
                m_Requested = false;
                return Payload;
            }

            return None;
        }

        // Returns if Requested without clearing the value
        [[nodiscard]] bool IsRequested() const { return m_Requested; }

    private:
        bool m_Requested = false;
        T Payload;
    };
}
