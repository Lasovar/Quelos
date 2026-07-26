#include "qspch.h"

#include "Components.h"
#include "Scene.h"

namespace Quelos {
    SharedPtr<Scene> SceneRoot::GetScene() const {
        return SharedAs<Scene>(m_Scene->shared_from_this());
    }
}
