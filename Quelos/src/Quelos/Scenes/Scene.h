#pragma once

#include "Entity.h"
#include <Quelos/Core/Ref.h>

#include "Actor.h"
#include "Quelos/Core/Event.h"
#include "Quelos/Renderer/FrameBuffer.h"

#include "ComponentRegistery.h"

namespace Quelos {
    enum class SystemGroup : uint8_t {
        OnStart,
        OnLoad,
        PostLoad,
        OnUpdate,
        PreUpdate,
        OnValidate,
        PostUpdate,
        PreStore,
        OnStore
    };

    constexpr flecs::entity_t SystemGroupToFlecsKind(const SystemGroup systemGroup) {
        switch (systemGroup) {
        case SystemGroup::OnStart: return flecs::OnStart;
        case SystemGroup::OnLoad: return flecs::OnLoad;
        case SystemGroup::PostLoad: return flecs::PostLoad;
        case SystemGroup::OnUpdate: return flecs::OnUpdate;
        case SystemGroup::PreUpdate: return flecs::PreUpdate;
        case SystemGroup::PostUpdate: return flecs::PostUpdate;
        case SystemGroup::OnValidate: return flecs::OnValidate;
        case SystemGroup::PreStore: return flecs::PreStore;
        case SystemGroup::OnStore: return flecs::OnStore;
        default: return flecs::OnUpdate;
        }
    }

    class Scene : public Asset {
    public:
        explicit Scene(std::string name = "Untitled Scene");

        template <typename Func>
        void Each(Func&& func) const {
            flecs::_::query_delegate<Func> f_delegate(m_World, FLECS_MOV(func));
        }

        template <typename... Comps, typename... Args, typename Func>
        void System(const SystemGroup systemGroup, Func&& func, Args&&... args) const {
            m_World.system<Comps...>(std::forward<Args>(args)...)
                   .kind(SystemGroupToFlecsKind(systemGroup))
                   .each(func);
        }

        template <typename... Comps, typename... Args, typename Func>
        void System(Func&& func, Args&&... args) const {
            System<Comps...>(
                SystemGroup::OnUpdate,
                std::forward<Func>(func),
                std::forward<Args>(args)...
            );
        }

        void Tick(float deltaTime) const;
        void StartRender(const Ref<FrameBuffer>& frameBuffer);
        void Render(uint32_t viewId) const;
        void EndRender();

        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string_view& name) { m_Name = name; }

        Actor CreateActor(std::string_view entityName);
        Actor CreateActor(const ActorID& guid, std::string_view entityName);
        void DestroyEntity(ActorID entityId);

        Entity GetSceneRoot() const { return m_SceneRoot; }
        void SetActorParentToRoot(const Actor& actor) const;

        void OnViewportResized(glm::vec2 viewportSize) const;

        flecs::world& GetWorld() { return m_World; }
        ComponentRegistry& GetComponentRegistry() { return m_ComponentRegistry; }

        Actor GetActor(const ActorID actorId) {
            if (!actorId) {
                return Actor(m_SceneRoot, actorId);
            }

            const auto it = m_ActorsMap.find(actorId);
            if (it == m_ActorsMap.end()) {
                return {};
            }

            return it->second;
        }

    public:
        static Ref<Scene> Copy(const Ref<Scene>& scene);

        static Ref<Scene> GetScene(const flecs::world& world) {
            return world.get<SceneRoot>().GetScene();
        }

        AssetType GetAssetType() const override { return GetStaticType(); }
        static AssetType GetStaticType() { return AssetType::Scene; }

        friend class SceneBinarySerializer;

    private:
        HashMap<ActorID, Actor> m_ActorsMap;
        ComponentRegistry m_ComponentRegistry;

        flecs::world m_World;
        std::string m_Name;

        flecs::entity m_SceneRoot;

        flecs::query<const WorldTransform&, const CameraComponent&> m_CameraQuery;
        flecs::query<const WorldTransform&, const MeshComponent&> m_RenderingQuery;

        bool m_SceneRenderStarted = false;

        flecs::entity m_TransformUpdate;
        flecs::entity m_TransformChildUpdate;
    };
}
