#pragma once

#include <utility>

#include "Quelos/Core/Base.h"
#include "Quelos/Core/Ref.h"

#include "Quelos/AssetManager/AssetManagerBase.h"

namespace Quelos {
    struct QS_API ProjectConfig {
        std::string ProjectName;

        OsPath ProjectPath;
        OsPath AssetsPath;
        OsPath SourcePath;
        OsPath LibraryPath;
        OsPath ProjectSettingsPath;
    };

    class QS_API Project {
    public:
        static bool IsLoaded() { return s_ActiveProject != nullptr; }

        static const OsPath& GetProjectPath() {
            return s_ActiveProject->m_Config.ProjectPath;
        }

        static const OsPath& GetProjectSettingsPath() {
            return s_ActiveProject->m_Config.ProjectSettingsPath;
        }

        static const OsPath& GetAssetsPath() {
            return s_ActiveProject->m_Config.AssetsPath;
        }

        static const OsPath& GetSourcePath() {
            return s_ActiveProject->m_Config.SourcePath;
        }

        static const OsPath& GetLibraryPath() {
            return s_ActiveProject->m_Config.LibraryPath;
        }

        static ProjectConfig& GetConfig() { return s_ActiveProject->m_Config; }

        static Ref<Project> Load(const ProjectConfig& projectConfig) {
            s_ActiveProject = CreateRef<Project>(projectConfig);

            if (!std::filesystem::exists(GetAssetsPath())) {
                std::filesystem::create_directories(GetAssetsPath());
            }

            if (!std::filesystem::exists(GetSourcePath())) {
                std::filesystem::create_directories(GetSourcePath());
            }

            if (!std::filesystem::exists(GetLibraryPath())) {
                std::filesystem::create_directories(GetLibraryPath());
            }

            if (!std::filesystem::exists(GetLibraryPath())) {
                std::filesystem::create_directories(GetProjectPath());
            }

            return s_ActiveProject;
        }

        [[nodiscard]] static const Ref<AssetManagerBase>& GetAssetManager() {
            return s_ActiveProject->m_AssetManager;
        }

        static void SetAssetManager(const Ref<AssetManagerBase>& assetManager) {
            s_ActiveProject->m_AssetManager = assetManager;
        }

    public:
        explicit Project(ProjectConfig projectConfig) : m_Config(std::move(projectConfig)) {
        }

    private:
        static Ref<Project> s_ActiveProject;

    private:
        ProjectConfig m_Config;
        Ref<AssetManagerBase> m_AssetManager;
    };
}
