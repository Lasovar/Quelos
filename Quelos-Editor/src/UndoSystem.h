#pragma once

#include "flecs.h"

#include "Quelos/Serialization/QuelArchive.h"
#include "Scene/SceneSerializer.h"

namespace Quelos {
    struct CommandVTable {
        void (*Apply)(void*);
        void (*Revert)(void*);
        void (*Destroy)(void*);
        void (*ApplyPatch)(SceneSerializer&, void*);
        void (*RemovePatch)(SceneSerializer&, void*);
    };

    struct CommandHeader {
        CommandVTable* VTable;
        uint32_t Size;
    };

    template <typename T>
    struct CommandWrapper {
        static void Apply(void* data) {
            static_cast<T*>(data)->Apply();
        }

        static void Revert(void* data) {
            static_cast<T*>(data)->Revert();
        }

        static void Destroy(void* data) {
            static_cast<T*>(data)->~T();
        }

        static void ApplyPatch(SceneSerializer& serializer, void* data) {
            serializer.Record(*static_cast<T*>(data));
        }

        static void RemovePatch(SceneSerializer& serializer, void* data) {
            serializer.Remove(*static_cast<T*>(data));
        }

        static CommandVTable VTABLE;
    };

    template <typename T>
    CommandVTable CommandWrapper<T>::VTABLE = {
        &CommandWrapper::Apply,
        &CommandWrapper::Revert,
        &CommandWrapper::Destroy,
        &CommandWrapper::ApplyPatch,
        &CommandWrapper::RemovePatch
    };

    class UndoSystem {
    public:
        UndoSystem() = default;

        explicit UndoSystem(SceneSerializer* sceneSerializer, const size_t capacity = 1024 * 1024)
            : m_SceneSerializer(sceneSerializer) {
            m_Buffer = CreateScope<byte[]>(capacity);
            m_Capacity = capacity;
            m_Head = 0;
        }

        UndoSystem(UndoSystem&& undoSystem) noexcept = default;
        UndoSystem& operator=(UndoSystem&&) noexcept = default;

        ~UndoSystem() = default;

        template <typename T, typename... Args>
        void Push(Args&&... args) {
            const size_t size = Align(sizeof(CommandHeader) + sizeof(T), alignof(std::max_align_t));

            if (size > m_Capacity) {
                QS_ERROR("Trying to push a command larger than the buffer: {} > {}", size, m_Capacity);
                return;
            }

            if (m_Head + size > m_Capacity) {
                m_Head = 0;

                while (!m_Stack.empty() && m_Tail < size) {
                    FreeOldest();
                }
            }

            auto* header = std::launder(reinterpret_cast<CommandHeader*>(m_Buffer.get() + m_Head));
            auto* data = reinterpret_cast<T*>(header + 1);

            new(data) T(std::forward<Args>(args)...);

            header->VTable = &CommandWrapper<T>::VTABLE;
            header->Size = sizeof(T);

            m_Stack.push_back(m_Head);

            m_Head += size;

            DestroyRedo();

            header->VTable->Apply(data);
            if (m_SceneSerializer) {
                header->VTable->ApplyPatch(*m_SceneSerializer, data);
            }
            else {
                QS_CORE_ERROR_TAG("UndoSystem", "SceneSerializer is null");
            }
        }

        void Undo() {
            if (m_Stack.empty()) {
                return;
            }

            const size_t offset = m_Stack.back();
            m_Stack.pop_back();

            auto* header = std::launder(reinterpret_cast<CommandHeader*>(m_Buffer.get() + offset));
            void* data = header + 1;

            header->VTable->Revert(data);

            if (m_SceneSerializer) {
                header->VTable->RemovePatch(*m_SceneSerializer, data);
            }
            else {
                QS_CORE_ERROR_TAG("UndoSystem", "SceneSerializer is null");
            }

            m_RedoStack.push_back(offset);
        }

        void Redo() {
            if (m_RedoStack.empty()) {
                return;
            }

            const size_t offset = m_RedoStack.back();
            m_RedoStack.pop_back();

            auto* header = std::launder(reinterpret_cast<CommandHeader*>(m_Buffer.get() + offset));
            void* data = header + 1;

            header->VTable->Apply(data);

            if (m_SceneSerializer) {
                header->VTable->ApplyPatch(*m_SceneSerializer, data);
            }
            else {
                QS_CORE_ERROR_TAG("UndoSystem", "SceneSerializer is null");
            }

            m_Stack.push_back(offset);
        }

        void DestroyRedo() {
            for (const size_t offset : m_RedoStack) {
                auto* header = std::launder(reinterpret_cast<CommandHeader*>(m_Buffer.get() + offset));
                void* data = header + 1;

                header->VTable->Destroy(data);
            }

            m_RedoStack.clear();
        }

    private:
        static size_t Align(const size_t size, const size_t alignment) {
            return (size + alignment - 1) & ~(alignment - 1);
        }

        void FreeOldest() {
            if (m_Stack.empty()) {
                return;
            }

            const size_t offset = m_Stack.front();
            m_Stack.pop_front();

            auto* header = std::launder(reinterpret_cast<CommandHeader*>(m_Buffer.get() + offset));
            void* data = header + 1;

            header->VTable->Destroy(data);

            const size_t size = Align(sizeof(CommandHeader) + header->Size, alignof(std::max_align_t));

            m_Tail = (offset + size) % m_Capacity;

            std::erase(m_RedoStack, offset);
        }

    private:
        Scope<byte[]> m_Buffer;
        size_t m_Capacity = 0;
        size_t m_Head = 0;
        size_t m_Tail = 0;

        Deque<size_t> m_Stack;
        Vec<size_t> m_RedoStack;

        SceneSerializer* m_SceneSerializer = nullptr;
    };
}
