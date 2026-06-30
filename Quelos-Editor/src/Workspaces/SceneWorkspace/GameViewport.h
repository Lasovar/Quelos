//
// Created by lasovar on 6/6/26.
//

#pragma once

#include "Panels/ViewportPanel.h"

namespace QuelosEditor {
    class SceneWorkspace;

    class GameViewport : public ViewportPanel {
    public:
        GameViewport(
            std::string name,
            SceneWorkspace& sceneWorkspace,
            RenderPassHandle renderPassHandle,
            RenderPassHandle shadowMaskHandle,
            uint32_t width,
            uint32_t height
        );

        void BeforeViewport() override;

    private:
        SceneWorkspace& m_SceneWorkspace;
    };
}
