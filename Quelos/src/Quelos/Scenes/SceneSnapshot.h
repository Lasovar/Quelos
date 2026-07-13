//
// Created by lasovar on 6/7/26.
//

#pragma once

#include "Quelos/Core/Base.h"

#include "Scene.h"

namespace Quelos {
    struct QS_API SceneHeader {
        uint32_t Magic = 0x53434E45; // 'SCNE'
        uint32_t Version = 1;

        uint64_t EntityCount = 0;

        uint64_t ComponentTypeCount = 0;
    };

    struct QS_API SceneSnapshot {
        std::string SceneName;
        Vec64<byte> Data{Allocator::Persistent};

        // Expects an already created empty scene
        void Load(const Ref<Scene>& scene) {
            Load(scene, SceneName, Data);
        }

        // Expects an already created empty scene
        static void Load(const Ref<Scene>& scene, const std::string& sceneName, BufferView data);
        static SceneSnapshot Create(const Ref<Scene>& scene);
    };
}
