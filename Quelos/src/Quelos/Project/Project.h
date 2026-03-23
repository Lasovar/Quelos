#pragma once

#include "Quelos/AssetManager/AssetManagerBase.h"
#include "Quelos/AssetManager/EditorAssetManager.h"
#include "Quelos/AssetManager/RuntimeAssetManager.h"
#include "Quelos/Core/Base.h"
#include "Quelos/Core/Ref.h"

namespace Quelos {
    struct ProjectConfig {
        std::string ProjectName;

        Path ProjectPath;
        Path AssetsPath;
        Path SourcePath;
        Path LibraryPath;
    };

    class Project {
    public:
        static bool IsLoaded() { return s_ActiveProject != nullptr; }

        static const Path& GetProjectPath() {
            return s_ActiveProject->m_Config.ProjectPath;
        }

        static const Path& GetAssetsPath() {
            return s_ActiveProject->m_Config.AssetsPath;
        }

        static const Path& GetSourcePath() {
            return s_ActiveProject->m_Config.SourcePath;
        }

        static const Path& GetLibraryPath() {
            return s_ActiveProject->m_Config.LibraryPath;
        }

        static ProjectConfig& GetConfig() { return s_ActiveProject->m_Config; }

        static Ref<Project> Load(const std::filesystem::path& projectPath) {
            ProjectConfig config;
            config.ProjectPath = projectPath;
            config.AssetsPath = projectPath / "Assets";
            config.SourcePath = projectPath / "Source";
            config.LibraryPath = projectPath / "Library";

            return Load(config);
        }

        static Ref<Project> Load(
            const Path& projectPath,
            const Path& assetsPath,
            const Path& sourcePath,
            const Path& libraryPath
        ) {
            ProjectConfig config;
            config.ProjectPath = projectPath;
            config.AssetsPath = assetsPath;
            config.SourcePath = sourcePath;
            config.LibraryPath = libraryPath;

            return Load(config);
        }

        static Ref<Project> Load(const ProjectConfig& projectConfig) {
            s_ActiveProject = CreateRef<Project>(projectConfig);
            s_ActiveProject->m_AssetManager = CreateRef<EditorAssetManager>();

            if (!std::filesystem::exists(GetAssetsPath())) {
                std::filesystem::create_directories(GetAssetsPath());
            }

            if (!std::filesystem::exists(GetSourcePath())) {
                std::filesystem::create_directories(GetSourcePath());
            }

            if (!std::filesystem::exists(GetLibraryPath())) {
                std::filesystem::create_directories(GetLibraryPath());
            }

            return s_ActiveProject;
        }

        [[nodiscard]] static Ref<AssetManagerBase> GetAssetManager() {
            return s_ActiveProject->m_AssetManager;
        }

        [[nodiscard]] static Ref<RuntimeAssetManager> GetRuntimeAssetManager() {
            return RefAs<RuntimeAssetManager>(s_ActiveProject->m_AssetManager);
        }

        [[nodiscard]] static Ref<EditorAssetManager> GetEditorAssetManager() {
            return RefAs<EditorAssetManager>(s_ActiveProject->m_AssetManager);
        }

    public:
        explicit Project(const ProjectConfig& projectConfig) : m_Config(projectConfig) {}

    private:
        inline static Ref<Project> s_ActiveProject;
    private:
        ProjectConfig m_Config;
		Ref<AssetManagerBase> m_AssetManager;
    };
}
