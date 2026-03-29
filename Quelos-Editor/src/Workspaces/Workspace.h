#pragma once

#include "Quelos/Core/Event.h"

namespace Quelos {
    class Workspace {
    public:
        Workspace() = default;
        virtual ~Workspace() = default;

        virtual void Tick(float deltaTime) = 0;
        virtual void OnImGuiRender(unsigned int dockspaceID) = 0;

        virtual void OnEvent(Event& event) = 0;
    };
}
