#pragma once

#include "Entity.h"
#include "Components.h"

#include "Quelos/Core/Log.h"

namespace Quelos {
    constexpr uint64_t k_OrderStep = 1ull << 32;

    class QS_API Actor : public Entity {
    public:
        Actor() = default;

        Actor(const flecs::entity id, const EntityID actorId) : Entity(id), m_EntityId(actorId) {
        }

        Actor(const Entity entity, const EntityID entityId) : Entity(entity), m_EntityId(entityId) { }

        Actor(const Entity entity) : Entity(entity) {
            if (auto* id = entity.TryGet<EntityID>()) {
                m_EntityId = *id;
            }
        }

        Actor(const flecs::entity id) : Entity(id) {
            if (auto* entityId = id.try_get<EntityID>()) {
                m_EntityId = *entityId;
            }
        }

        void SetParent(const Actor parent) const {
            if (m_ID.world().is_deferred()) {
                Entity::SetParent(static_cast<Entity>(parent));
            } else {
                OrderBack(parent);
            }
        }

        [[nodiscard]] Actor GetParent() const {
            return m_ID.parent();
        }

        void RemoveParent() const {
            SetParent(Actor(m_ID.world().singleton<SceneRoot>(), EntityID()));
        }

        void OrderBefore(const Actor target) const {
            const flecs::entity parent = target.GetInternalID().parent();
            Vec<flecs::entity> children;

            parent.children([&](const flecs::entity child) {
                children.push_back(child);
            });

            uint64_t prev = 0;
            const uint64_t next = target.Get<ChildOrder>().Value;

            for (size_t i = 0; i < children.size(); ++i) {
                if (children[i] == target.GetInternalID() && i > 0) {
                    prev = children[i - 1].get<ChildOrder>().Value;
                    break;
                }
            }

            const uint64_t newOrder = prev == 0 ? next / 2 : prev + (next - prev) / 2;

            m_ID.add(flecs::ChildOf, parent);
            m_ID.set<ChildOrder>({newOrder});

            Actor(parent, EntityID()).IndexChildOrders();
        }

        void OrderAfter(const Actor target) const {
            const flecs::entity parent = target.GetInternalID().target(flecs::ChildOf);
            Vec<flecs::entity> children;

            parent.children([&](const flecs::entity c) {
                children.push_back(c);
            });

            const uint64_t prev = target.Get<ChildOrder>().Value;
            uint64_t next = prev + k_OrderStep;

            for (size_t i = 0; i < children.size(); ++i) {
                if (children[i] == target.GetInternalID() && i + 1 < children.size()) {
                    next = children[i + 1].get<ChildOrder>().Value;
                    break;
                }
            }

            const uint64_t newOrder =  prev + (next - prev) / 2;

            m_ID.add(flecs::ChildOf, parent);
            m_ID.set<ChildOrder>({newOrder});

            Actor(parent, EntityID()).IndexChildOrders();
        }


        void OrderFront(const Actor parent) const {
            uint64_t minOrder = UINT64_MAX;

            parent.GetInternalID().children([&](const flecs::entity c) {
                if (auto* childOrder = c.try_get<ChildOrder>()) {
                    minOrder = std::min(minOrder, childOrder->Value);
                }
            });

            const uint64_t newOrder = minOrder == UINT64_MAX ? k_OrderStep : minOrder / 2;

            m_ID.add(flecs::ChildOf, parent.GetInternalID());
            m_ID.set<ChildOrder>({newOrder});

            parent.IndexChildOrders();
        }

        void OrderFront() const {
            OrderFront(GetParent());
        }

        void OrderBack(const Actor& parent) const {
            uint64_t maxOrder = 0;
            parent.GetInternalID().children([&](const flecs::entity child) {
                if (const auto childOrder = child.try_get<ChildOrder>()) {
                    maxOrder = std::max(maxOrder, childOrder->Value);
                }
            });

            const uint64_t newOrder = maxOrder == 0 ? k_OrderStep : maxOrder + k_OrderStep;

            m_ID.add(flecs::ChildOf, parent.GetInternalID());
            m_ID.set<ChildOrder>({newOrder});

            parent.IndexChildOrders();
        }

        void OrderBack() const {
            SetParent(GetParent());
        }

        void IndexChildOrders() const {
            if (m_ID.world().is_deferred()) {
                return;
            }

            Vec<Pair<flecs::entity, ChildOrder>> children;

            m_ID.children([&](const flecs::entity c) {
                children.push_back({c, c.get<ChildOrder>()});
            });

            if (children.empty()) {
                return;
            }

            std::ranges::sort(
                children,
                [](const Pair<flecs::entity, ChildOrder>& first, const Pair<flecs::entity, ChildOrder>& second) {
                    return first.second.Value < second.second.Value;
                });

            Vec<flecs::entity_t> ids;
            ids.reserve(children.size());

            std::ranges::transform(children,
                std::back_inserter(ids),
                [](const Pair<flecs::entity, ChildOrder>& e) { return e.first; }
            );

            std::string name = GetName();
            m_ID.set_child_order(ids.data(), ids.size());
        }

        EntityID GetActorID() const { return m_EntityId; }

    private:
        EntityID m_EntityId;
    };
}


namespace std {
    template <>
    struct hash<Quelos::Actor> {
        size_t operator()(const Quelos::Actor& actor) const noexcept {
            return actor.GetInternalID();
        }
    };
}
