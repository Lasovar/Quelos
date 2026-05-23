#include <qspch.h>
#include "Entity.h"
#include "Quelos/ImGui/ImGuiUI.h"

namespace Quelos {
    bool Entity::IsAlive() const {
        return m_ID.is_alive();
    }

    void Entity::Destruct() const {
        m_ID.destruct();
    }

    void Entity::SetName(const std::string_view name) const {
        m_ID.set_doc_name(UI::FormatTemp("{}", name));
    }
}
