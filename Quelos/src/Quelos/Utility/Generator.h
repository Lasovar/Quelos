#pragma once

#include <optional>
#include <coroutine>

namespace Quelos {
    template<typename T>
    class Generator {
    public:
        struct promise_type {
            std::optional<T> current;

            Generator get_return_object() {
                return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            static std::suspend_always initial_suspend() noexcept { return {}; }
            static std::suspend_always final_suspend() noexcept { return {}; }
            static void return_void() noexcept {}
            static void unhandled_exception() { std::terminate(); }

            std::suspend_always yield_value(T value) noexcept {
                current = std::move(value);
                return {};
            }
        };

        using CoroutineHandle = std::coroutine_handle<promise_type>;

        explicit Generator(CoroutineHandle handle)
            : m_Handle(handle)
        {
        }

        Generator(const Generator&) = delete;
        Generator(Generator&& other) noexcept
            : m_Handle(other.m_Handle)
        {
            other.m_Handle = {};
        }

        ~Generator() {
            if (m_Handle) {
                m_Handle.destroy();
            }
        }

        class iterator {
        public:
            explicit iterator(CoroutineHandle handle)
                : m_Handle(handle)
            {
                advance();
            }

            iterator& operator++() { advance(); return *this; }
            const T& operator*() const { return *m_Handle.promise().current; }
            bool operator==(std::default_sentinel_t) const { return !m_Handle || m_Handle.done(); }

        private:
            void advance() {
                if (m_Handle) {
                    m_Handle.resume();
                }

                if (m_Handle && m_Handle.done()) {
                    m_Handle = {};
                }
            }

            CoroutineHandle m_Handle{};
        };

        iterator begin() { return iterator{m_Handle}; }
        static std::default_sentinel_t end() { return {}; }

    private:
        CoroutineHandle m_Handle;
    };
}
