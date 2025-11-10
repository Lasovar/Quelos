#include <qspch.h>
#include "Entity.h"

namespace Quelos {

	bool Entity::IsAlive() const {
		return m_ID.is_alive();
	}

	void Entity::Destruct() {
		m_ID.destruct();
	}

}

