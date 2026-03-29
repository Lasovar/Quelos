#include "qspch.h"
#include "ProjectSerializer.h"

#include "Quelos/Project/Project.h"
#include "Quelos/Serialization/QuelArchive.h"

namespace QuelosEditor {
    ProjectSerializer::ProjectSerializer(const Path& projectPath) {
        using namespace Serialization;
        std::string projectName = projectPath.filename().string();
        Path configFilePath = projectPath / (projectName + ".quel");

        if (!std::filesystem::exists(projectPath)) {
            std::filesystem::create_directories(projectPath);
        }

        if (!std::filesystem::exists(configFilePath)) {
            std::string buffer;
            StringQuelWriter quelWriter(buffer);

            quelWriter.SetIndent(2);

            quelWriter.Write(SectionEvent{"project"});
            quelWriter.WriteField("name", projectName);
            quelWriter.CloseSection();

            quelWriter.WriteField("assets", "Assets/");
            quelWriter.WriteField("source", "Source/");
            quelWriter.WriteField("library", "Library/");
            quelWriter.WriteField("projectSettings", "ProjectSettings/");

            std::ofstream projectConfig(configFilePath, std::ios::binary);
            if (projectConfig.write(buffer.data(), buffer.size())) {
                QS_ERROR_TAG("ProjectSerializer", "Failed to create default config at '{}'", configFilePath.string());
            }
        }

        std::ifstream projectConfig(configFilePath, std::ios::binary | std::ios::ate);
        if (!projectConfig) {
            QS_ERROR_TAG("ProjectSerializer", "Failed to open config at '{}'", configFilePath.string());
            return;
        }

        const size_t patchFileSize = projectConfig.tellg();
        projectConfig.seekg(0);

        std::string buffer;
        buffer.resize(patchFileSize);
        projectConfig.read(buffer.data(), patchFileSize);

        QuelReader reader(buffer);

        bool breakFlag = false;
        std::string_view currentField;

        ProjectConfig config;
        config.ProjectPath = std::filesystem::absolute(projectPath);

        for (auto&& parserEvent : reader.Parse()) {
            if (breakFlag) {
                break;
            }

            std::visit([&]<typename TEvent>(const TEvent& e) {
                using T = std::decay_t<TEvent>;

                if constexpr (std::is_same_v<T, SectionEvent>) {
                    if (e.Name != "project") {
                        breakFlag = true;
                        QS_ERROR_TAG(
                            "ProjectSerializer",
                            "Failed to load project at '{}'! config file isn't a project config",
                            projectPath.string()
                        );

                        return;
                    }
                }
                else if constexpr (std::is_same_v<T, ComponentEvent>) {
                }
                else if constexpr (std::is_same_v<T, FieldEvent>) {
                    currentField = e.Path;
                }
                else if constexpr (std::is_same_v<T, ValueEvent>) {
                    std::string_view value;
                    if (const std::string_view* valueResult = std::get_if<std::string_view>(&e.Value)) {
                        value = *valueResult;
                    }
                    else {
                        return;
                    }

                    if (currentField == "name") {
                        config.ProjectName = value;
                    }
                    else if (currentField == "assets") {
                        config.AssetsPath = config.ProjectPath / value;
                    }
                    else if (currentField == "source") {
                        config.SourcePath = config.ProjectPath / value;
                    }
                    else if (currentField == "library") {
                        config.LibraryPath = config.ProjectPath / value;
                    }
                    else if (currentField == "projectSettings") {
                        config.ProjectSettingsPath = config.ProjectPath / value;
                    }
                }
                else if constexpr (std::is_same_v<T, TupleBeginEvent>) {
                }
                else if constexpr (std::is_same_v<T, TupleEndEvent>) {
                }
                else if constexpr (std::is_same_v<T, ArrayBeginEvent>) {
                }
                else if constexpr (std::is_same_v<T, ArrayEndEvent>) {
                }
            }, parserEvent);
        }

        Project::Load(config);

        m_AssetManager = CreateRef<EditorAssetManager>();
        m_AssetManager->DeserializeAssetRegistry();

        Project::SetAssetManager(m_AssetManager);
    }

    ProjectSerializer::~ProjectSerializer() {
        Serialize();
    }

    void ProjectSerializer::Serialize() const {
        m_AssetManager->SerializeAssetRegistry();
    }
}
