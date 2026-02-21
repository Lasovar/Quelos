#pragma once

namespace Quelos {
    class Scene;
}

namespace Quelos::Serialization {    struct SceneHeader {
        uint32_t Magic = 0x53434E45; // 'SCNE'
        uint32_t Version = 1;

        uint64_t EntityCount = 0;

        uint64_t ComponentTypeCount = 0;
    };

    class SceneBinarySerializer {
        public:
        static void Deserialize(const Ref<Scene>& scene, const std::filesystem::path& path);
        static void Serialize(const Ref<Scene>& scene, const std::filesystem::path& path);
    };
}
