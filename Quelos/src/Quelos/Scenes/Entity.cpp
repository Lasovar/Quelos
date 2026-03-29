#include <qspch.h>
#include "Entity.h"

namespace Quelos {
    bool Entity::IsAlive() const {
        return m_ID.is_alive();
    }

    void Entity::Destruct() const {
        m_ID.destruct();
    }

    static bool NameExists(const flecs::world& world, const char* name) {
        return world.lookup(name).is_valid();
    }

    static bool NameExists(const flecs::entity& parent, const char* name) {
        return parent.lookup(name).is_valid();
    }

    void Entity::SetName(const std::string_view name) const {
        constexpr size_t k_MaxStack = 64;

        const char* base = nullptr;

        if (name.size() < k_MaxStack) {
            static char buffer[k_MaxStack];
            std::memcpy(buffer, name.data(), name.size());
            buffer[name.size()] = '\0';
            base = buffer;
        }
        else {
            const std::string temp(name);
            base = temp.c_str();
        }

        if (const flecs::entity parent = m_ID.parent(); parent.is_valid()) {
            if (NameExists(parent, base)) {
                int i = 1;
                std::string uniqueName;

                do {
                    uniqueName = fmt::format("{}_{}", base, i++);
                }
                while (NameExists(parent, uniqueName.c_str()));

                m_ID.set_name(uniqueName.c_str());
            }
            else {
                m_ID.set_name(base);
            }
        } else {
            if (const flecs::world& world = m_ID.world(); NameExists(world, base)) {
                int i = 1;
                std::string uniqueName;

                do {
                    uniqueName = std::format("{}_{}", base, i++);
                }
                while (NameExists(world, uniqueName.c_str()));

                m_ID.set_name(uniqueName.c_str());
            }
            else {
                m_ID.set_name(base);
            }
        }
    }
}
