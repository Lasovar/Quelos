#pragma once

#include "flecs.h"

#include "Quelos/Serialization/QuelArchive.h"
#include "Scene/SceneSerializer.h"

namespace QuelosEditor {
    using namespace Quelos;

    template<typename T>
    concept IsSceneCommand = requires(const T& cmd) {
        { cmd.Scene } -> std::convertible_to<Ref<Scene>>;
    };

    class UndoSystem;

    struct CommandVTable {
        void (*Apply)(void*);
        void (*Revert)(void*);
        void (*Destroy)(void*);
        void (*ApplyPatch)(UndoSystem&, void*);
        void (*RemovePatch)(UndoSystem&, void*);
    };

    struct CommandHeader {
        CommandVTable* VTable = nullptr;
        bool IsSceneCommand = false;
        uint32_t Size = 0;
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

        static CommandVTable VTABLE;
    };

    template <typename T>
    CommandVTable CommandWrapper<T>::VTABLE = {
        &CommandWrapper::Apply,
        &CommandWrapper::Revert,
        &CommandWrapper::Destroy,
        nullptr,
        nullptr
    };

    class UndoSystem {
    public:
        UndoSystem(const uint64_t capacity = 1024 * 1024) {
            if (!s_Instance) {
                s_Instance = this;
            }

            m_Buffer = Buffer::Allocate(capacity);
            m_Capacity = capacity;
            m_Head = 0;
        }

        UndoSystem(UndoSystem&& undoSystem) noexcept = default;
        UndoSystem& operator=(UndoSystem&&) noexcept = default;

        ~UndoSystem() = default;

        static UndoSystem& Get() { return *s_Instance; }

        template <typename T, typename... Args>
        void Push(Args&&... args) {
            const uint64_t size = Align(sizeof(CommandHeader) + sizeof(T), alignof(std::max_align_t));

            if (size > m_Capacity) {
                QS_ERROR("Trying to push a command larger than the buffer: {} > {}", size, m_Capacity);
                return;
            }

            if (m_Head + size > m_Capacity) {
                m_Head = 0;

                while (!m_UndoStack.empty() && m_Tail < size) {
                    FreeOldest();
                }
            }

            auto* header = std::launder(reinterpret_cast<CommandHeader*>(m_Buffer.data() + m_Head));
            auto* data = reinterpret_cast<T*>(header + 1);

            new(data) T(std::forward<Args>(args)...);

            header->VTable = &CommandWrapper<T>::VTABLE;
            header->Size = sizeof(T);


            m_UndoStack.push_back(m_Head);

            m_Head += size;

            DestroyRedo();

            header->VTable->Apply(data);

            if constexpr (IsSceneCommand<T>) {
                header->IsSceneCommand = true;

                header->VTable->ApplyPatch = [](UndoSystem& undoSystem, void* commandData) {
                    T* cmd = static_cast<T*>(commandData);
                    auto it = undoSystem.m_SceneSerializers.find(cmd->Scene->GetAssetID());
                    if (it != undoSystem.m_SceneSerializers.end()) {
                        if (SceneSerializer* sceneSerializer = it->second) {
                            sceneSerializer->Record(*cmd);
                        }
                    }
                };

                header->VTable->RemovePatch = [](UndoSystem& undoSystem, void* commandData) {
                    T* cmd = static_cast<T*>(commandData);
                    auto it = undoSystem.m_SceneSerializers.find(cmd->Scene->GetAssetID());
                    if (it != undoSystem.m_SceneSerializers.end()) {
                        if (SceneSerializer* sceneSerializer = it->second) {
                            sceneSerializer->Remove(*cmd);
                        }
                    }
                };

                header->VTable->ApplyPatch(*this, data);
            }
        }

        void Undo() {
            if (m_UndoStack.empty()) {
                return;
            }

            const uint64_t offset = m_UndoStack.back();
            m_UndoStack.pop_back();

            auto* header = std::launder(reinterpret_cast<CommandHeader*>(m_Buffer.data() + offset));
            void* data = header + 1;

            header->VTable->Revert(data);

            if (header->IsSceneCommand) {
                header->VTable->RemovePatch(*this, data);
            }

            m_RedoStack.push_back(offset);
        }

        void Redo() {
            if (m_RedoStack.empty()) {
                return;
            }

            const uint64_t offset = m_RedoStack.back();
            m_RedoStack.pop_back();

            auto* header = std::launder(reinterpret_cast<CommandHeader*>(m_Buffer.data() + offset));
            void* data = header + 1;

            header->VTable->Apply(data);

            if (header->IsSceneCommand) {
                header->VTable->ApplyPatch(*this, data);
            }

            m_UndoStack.push_back(offset);
        }

        void DestroyRedo() {
            for (const uint64_t offset : m_RedoStack) {
                auto* header = std::launder(reinterpret_cast<CommandHeader*>(m_Buffer.data() + offset));
                void* data = header + 1;

                header->VTable->Destroy(data);
            }

            m_RedoStack.clear();
        }

        void AddSceneSerializer(const Ref<Scene>& scene, SceneSerializer* sceneSerializer) {
            m_SceneSerializers[scene->GetAssetID()] = sceneSerializer;
        }

        void RemoveSceneSerializer(const Ref<Scene>& scene) {
            m_SceneSerializers.erase(scene->GetAssetID());
        }

    private:
        static uint64_t Align(const uint64_t size, const uint64_t alignment) {
            return (size + alignment - 1) & ~(alignment - 1);
        }

        void FreeOldest() {
            if (m_UndoStack.empty()) {
                return;
            }

            const uint64_t offset = m_UndoStack.front();
            m_UndoStack.pop_front();

            auto* header = std::launder(reinterpret_cast<CommandHeader*>(m_Buffer.data() + offset));
            void* data = header + 1;

            header->VTable->Destroy(data);

            const uint64_t size = Align(sizeof(CommandHeader) + header->Size, alignof(std::max_align_t));

            m_Tail = (offset + size) % m_Capacity;

            std::erase(m_RedoStack, offset);
        }

    private:
        static UndoSystem* s_Instance;
    private:
        Buffer m_Buffer;
        uint64_t m_Capacity = 0;
        uint64_t m_Head = 0;
        uint64_t m_Tail = 0;

        Deque<uint64_t> m_UndoStack;
        Vec<uint64_t> m_RedoStack;

        HashMap<AssetID, SceneSerializer*> m_SceneSerializers;
    };
}
