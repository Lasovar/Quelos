#pragma once

#include "Entity.h"
#include <Quelos/Core/Ref.h>

#include "Actor.h"
#include "Quelos/Renderer/FrameBuffer.h"

#include "ComponentRegistery.h"
#include "WorldRenderer.h"

namespace Quelos {
    enum class QS_API SystemGroup : uint8_t {
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

    struct QS_API LocalToWorldTransformSystem {
        explicit LocalToWorldTransformSystem(const flecs::world& world) {
            world.component<LocalTransform>();
            world.component<WorldTransform>();

            world.observer<LocalTransform>()
                 .event(flecs::OnAdd)
                 .each([](const flecs::entity e, const LocalTransform&) {
                     e.ensure<WorldTransform>();
                 });

            TransformUpdate = world.entity("TransformUpdate")
                                       .add(flecs::Phase)
                                       .depends_on(flecs::OnUpdate);

            TransformChildUpdate = world.entity("TransformChildUpdate")
                                            .add(flecs::Phase)
                                            .depends_on(TransformUpdate);

            world.system<const LocalTransform&, WorldTransform&>()
                             .multi_threaded()
                             .without(flecs::ChildOf, flecs::Wildcard)
                             .kind(flecs::OnUpdate)
                             .each([](const LocalTransform& local, WorldTransform& transform) {
                                 transform.Value = mathExt::srt(local.Scale, local.Rotation, local.Position);
                             });

            world.system<const LocalTransform&, WorldTransform&, const WorldTransform&>()
                                  .multi_threaded()
                                  .with(flecs::ChildOf, flecs::Wildcard)
                                  .term_at(2).cascade()
                                  .kind(TransformChildUpdate)
                                  .each([](
                                      const LocalTransform& local,
                                      WorldTransform& transform,
                                      const WorldTransform& parentWorld
                                  ) {
                                          transform.Value = math::mul(
                                              parentWorld.Value,
                                              mathExt::srt(local.Scale, local.Rotation, local.Position)
                                          );
                                  });

        }

        flecs::entity TransformUpdate;
        flecs::entity TransformChildUpdate;
    };

    class QS_API Scene : public Asset, public std::enable_shared_from_this<Scene> {
    public:
        explicit Scene(flecs::world& world, std::string name = "Untitled Scene");

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

        Pair<float4x4, float4x4> GetViewAndProjection() const;
        RenderViewParams GetRenderViewParams() const;

        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string_view& name) { m_Name = name; }

        Actor CreateActor(std::string_view entityName);
        Actor CreateActor(const EntityID& guid, std::string_view entityName);
        Actor CreateActor(const EntityID& guid, std::string_view entityName, EntityID parent);

        Entity CreateEntity(std::string_view entityName);
        Entity CreateEntity(const EntityID& guid, std::string_view entityName);
        Entity CreateEntity(const EntityID& guid, std::string_view entityName, EntityID parent);
        void DestroyEntity(EntityID entityId);

        Entity GetSceneRoot() const { return m_SceneRoot; }
        void SetActorParentToRoot(const Actor& actor) const;

        void OnViewportResized(float2 viewportSize) const;

        void Init();

        void Disable() const;
        void Destroy();

        flecs::world& GetWorld() { return m_World; }

        Entity GetEntity(const EntityID entityId) {
            if (!entityId) {
                return m_SceneRoot;
            }

            const auto it = m_EntitiesMap.find(entityId);
            if (it == m_EntitiesMap.end()) {
                return {};
            }

            return it->second;
        }

        Actor GetActor(const EntityID actorId) {
            if (!actorId) {
                return { m_SceneRoot, actorId };
            }

            const auto it = m_ActorsMap.find(actorId);
            if (it == m_ActorsMap.end()) {
                return {};
            }

            return it->second;
        }

    public:
        static Ref<Scene> GetScene(const flecs::world& world) {
            return world.get<SceneRoot>().GetScene();
        }

        const AssetType& GetAssetType() const override { return GetStaticType(); }
        static const AssetType& GetStaticType() {
            static AssetType assetType = Quelos::GetAssetType<Scene>();
            return assetType;
        }

        friend class SceneBinarySerializer;

    private:
        HashMap<EntityID, Actor> m_ActorsMap;
        HashMap<EntityID, Entity> m_EntitiesMap;

        flecs::world& m_World;

        std::string m_Name;

        flecs::entity m_SceneRoot;

        flecs::query<const WorldTransform&, const CameraComponent&> m_CameraQuery;

        bool m_SceneRenderStarted = false;
    };
}
