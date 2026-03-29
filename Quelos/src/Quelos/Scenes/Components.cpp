#include "qspch.h"

#include "Scene.h"

namespace Quelos {
    Ref<Scene> SceneRoot::GetScene() const {
        return RefAs<Scene>(m_Scene->shared_from_this());
    }
}
