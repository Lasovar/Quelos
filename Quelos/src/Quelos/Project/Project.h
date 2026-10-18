#pragma once

#include <utility>

#include "Quelos/Core/Base.h"
#include "Quelos/Core/SmartPointers.h"

#include "Quelos/AssetManager/AssetManagerBase.h"

namespace Quelos {
    struct QS_API ProjectConfig {
        std::string ProjectName;

        OsPath ProjectPath;
        String ProjectPathStr;
        OsPath AssetsPath;
        String AssetsPathStr;
        OsPath SourcePath;
        OsPath LibraryPath;
        String LibraryPathStr;
        OsPath ProjectSettingsPath;
    };

    class QS_API Project {
    public:
        static bool IsLoaded() { return s_ActiveProject != nullptr; }

        static const OsPath& GetProjectPath() {
            return s_ActiveProject->m_Config.ProjectPath;
        }

        static std::string_view GetProjectPathView() {
            return s_ActiveProject->m_Config.ProjectPathStr;
        }

        static const OsPath& GetProjectSettingsPath() {
            return s_ActiveProject->m_Config.ProjectSettingsPath;
        }

        static const OsPath& GetAssetsPath() {
            return s_ActiveProject->m_Config.AssetsPath;
        }

        static std::string_view GetAssetsPathView() {
            return s_ActiveProject->m_Config.AssetsPathStr;
        }

        static const OsPath& GetSourcePath() {
            return s_ActiveProject->m_Config.SourcePath;
        }

        // Not guaranteed to be accessible in runtime
        static const OsPath& GetLibraryPath() {
            return s_ActiveProject->m_Config.LibraryPath;
        }

        static std::string_view GetLibraryPathView() {
            return s_ActiveProject->m_Config.LibraryPathStr;
        }

        // Accessible in runtime
        static const OsPath& GetCookedAssetsPath() {
            return s_ActiveProject->m_CookedAssetsPath;
        }

        static ProjectConfig& GetConfig() { return s_ActiveProject->m_Config; }

        static SharedPtr<Project> Load(ProjectConfig&& projectConfig) {
            s_ActiveProject = CreateShared<Project>(std::move(projectConfig));

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

            if (!std::filesystem::exists(GetCookedAssetsPath())) {
                std::filesystem::create_directories(GetCookedAssetsPath());
            }

            return s_ActiveProject;
        }

        [[nodiscard]] static const SharedPtr<AssetManagerBase>& GetAssetManager() {
            return s_ActiveProject->m_AssetManager;
        }

        static void SetAssetManager(const SharedPtr<AssetManagerBase>& assetManager) {
            s_ActiveProject->m_AssetManager = assetManager;
        }

    public:
        explicit Project(ProjectConfig projectConfig) : m_Config(std::move(projectConfig)) {
            m_CookedAssetsPath = m_Config.LibraryPath / "CookedAssets";
        }

    private:
        static SharedPtr<Project> s_ActiveProject;

    private:
        ProjectConfig m_Config;
        OsPath m_CookedAssetsPath;
        SharedPtr<AssetManagerBase> m_AssetManager;
    };
}
