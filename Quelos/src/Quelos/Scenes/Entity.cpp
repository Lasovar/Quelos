#include <qspch.h>
#include "Entity.h"

#include "Components.h"
#include "Quelos/ImGui/ImGuiUI.h"

namespace Quelos {
    bool Entity::IsAlive() const {
        return m_ID.is_alive();
    }

    void Entity::Destruct() const {
        m_ID.destruct();
    }

    void Entity::SetName(const std::string_view name) const {
        m_ID.set_doc_name(FormatTemp("{}", name));
    }

    void Entity::RemoveParent() const {
        if (const flecs::entity scene = m_ID.target<Owner>()) {
            m_ID.child_of(scene);
        } else {
            m_ID.remove(flecs::ChildOf, flecs::Wildcard);
        }
    }

    void Entity::OrderBack(const Entity entity, const Entity& parent) {
        uint64_t maxOrder = 0;
        parent.GetInternalID().children([&](const flecs::entity child) {
            if (const auto childOrder = child.try_get<ChildOrder>()) {
                maxOrder = std::max(maxOrder, childOrder->Value);
            }
        });

        const uint64_t newOrder = maxOrder == 0 ? k_OrderStep : maxOrder + k_OrderStep;

        entity.m_ID.add(flecs::ChildOf, parent.GetInternalID());
        entity.m_ID.set<ChildOrder>({newOrder});

        //TODO: parent.IndexChildOrders();
    }
}
