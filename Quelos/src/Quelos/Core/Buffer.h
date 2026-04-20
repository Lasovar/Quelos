#pragma once

#include "Base.h"

namespace Quelos {
    struct QS_API Buffer {
        using Deleter = void(*)(void*);

        Buffer() = default;
        Buffer(byte* data, const uint64_t size, const Deleter deleter)
            : m_Data(data), m_Size(size), m_Deleter(deleter) {}

        static Buffer Allocate(const uint64_t size) {
            void* ptr = std::malloc(size);
            return { static_cast<byte*>(ptr), size, std::free };
        }

        static Buffer Adopt(void* data, const uint64_t size, const Deleter deleter) {
            return { static_cast<byte*>(data), size, deleter };
        }

        static Buffer Copy(const void* data, const uint64_t size) {
            Buffer buffer = Allocate(size);
            std::memcpy(buffer.m_Data, data, size);
            return buffer;
        }

        static Buffer Copy(const Buffer& other) {
            return Copy(other.m_Data, other.m_Size);
        }

        Buffer(Buffer&& other) noexcept {
            *this = std::move(other);
        }

        Buffer& operator=(Buffer&& other) noexcept {
            if (this != &other) {
                release();

                m_Data = other.m_Data;
                m_Size = other.m_Size;
                m_Deleter = other.m_Deleter;

                other.m_Data = nullptr;
                other.m_Size = 0;
                other.m_Deleter = nullptr;
            }

            return *this;
        }

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        ~Buffer() {
            release();
        }

        void release() {
            if (m_Data && m_Deleter) {
                m_Deleter(m_Data);
            }

            release_ownership();
        }

        void release_ownership() {
            m_Data = nullptr;
            m_Size = 0;
            m_Deleter = nullptr;
        }

        [[nodiscard]] BufferView view() const {
            return { m_Data, m_Size };
        }

        MutBufferView mut_view() {
            return { m_Data, m_Size };
        }

        [[nodiscard]] byte* data() const {
            return m_Data;
        }

        [[nodiscard]] uint64_t size() const {
            return m_Size;
        }

        Deleter deleter() const {
            return m_Deleter;
        }

        template <typename T>
        T* as() {
            if (!m_Data) {
                return nullptr;
            }

            if (m_Size < sizeof(T) || reinterpret_cast<uintptr_t>(m_Data) % alignof(T) != 0) {
                return nullptr;
            }

            return reinterpret_cast<T*>(m_Data);
        }

        template <typename T>
        const T* as() const {
            if (!m_Data) {
                return nullptr;
            }

            if (m_Size < sizeof(T) || reinterpret_cast<uintptr_t>(m_Data) % alignof(T) != 0) {
                return nullptr;
            }

            return reinterpret_cast<const T*>(m_Data);
        }


        template <typename T>
        std::optional<T> as_value(const uint64_t offset = 0) {
            if (offset + sizeof(T) > m_Size) {
                return std::nullopt;
            }

            T value;
            std::memcpy(&value, m_Data + offset, sizeof(T));
            return value;
        }

        explicit operator bool() const {
            return m_Data != nullptr;
        }

    private:
        byte* m_Data = nullptr;
        uint64_t m_Size = 0;
        Deleter m_Deleter = nullptr;
    };
}
