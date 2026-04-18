#include "qspch.h"
#include "ModelMetadataHandler.h"

#include "Quelos/AssetManager/AssetImporter.h"
#include "Quelos/Project/Project.h"
#include "Quelos/Serialization/Serializer.h"

namespace QuelosEditor {
    using namespace Quelos;

    static ModelMetadataHandler s_ModelHandler;

    void RegisterModelMetadataHandler() {
        AssetMetadataHandlerRegistry::Instance().RegisterHandler(
            AssetMetadataHandler<ModelMetadataHandler>::Create(s_ModelHandler)
        );
    }

    std::optional<AssetHandle> ModelMetadataHandler::ReadAssetHandle(const std::string_view assetPath) {
        std::string quelPath = GetMetadataFilePath(assetPath);
        return ReadFromQuelFile(quelPath);
    }

    bool ModelMetadataHandler::WriteAssetHandle(const std::string_view assetPath, const AssetHandle& handle) {
        std::string quelPath = GetMetadataFilePath(assetPath);
        return WriteToQuelFile(quelPath, handle);
    }

    bool ModelMetadataHandler::SupportsAssetPath(const std::string_view assetPath) {
        const std::string_view extension = Extension(assetPath);
        return extension == ".fbx"/* || extension == ".obj" || extension == ".gltf" || extension == ".glb"*/;
    }

    std::string ModelMetadataHandler::GetMetadataFilePath(const std::string_view assetPath) {
        OsPath assetFile(assetPath);
        if (assetFile.is_absolute()) {
            assetFile = std::filesystem::relative(assetFile, Project::GetProjectPath());
        }
        return (Project::GetProjectPath() / (assetFile.generic_string() + ".quel")).generic_string();
    }

    std::optional<AssetHandle> ModelMetadataHandler::ReadFromQuelFile(const std::string_view quelPath) {
        using namespace Serialization;

        OsPath metadataPath = quelPath;
        if (!std::filesystem::exists(metadataPath)) {
            return std::nullopt;
        }

        std::ifstream file(metadataPath, std::ios::binary | std::ios::ate);
        if (!file) {
            return std::nullopt;
        }

        const size_t fileSize = file.tellg();
        file.seekg(0);

        std::string buffer;
        buffer.resize(fileSize);
        file.read(buffer.data(), static_cast<std::streamsize>(fileSize));

        QuelReader reader(buffer);

        std::string_view currentField;
        std::optional<AssetHandle> modelHandle;

        for (auto&& parserEvent : reader.Parse()) {
            std::visit([&]<typename TEvent>(const TEvent& e) {
                using T = std::decay_t<TEvent>;

                if constexpr (std::is_same_v<T, SectionEvent>) {
                    // We only care about the Model section for the main asset handle
                }
                else if constexpr (std::is_same_v<T, FieldEvent>) {
                    currentField = e.Path;
                }
                else if constexpr (std::is_same_v<T, ValueEvent>) {
                    if (const std::string_view* valueResult = std::get_if<std::string_view>(&e.Value)) {
                        if (currentField == "handle") {
                            modelHandle = AssetHandle(*valueResult);
                        }
                    }
                }
            }, parserEvent);

            // Stop early if we found the handle
            if (modelHandle) {
                break;
            }
        }

        return modelHandle;
    }

    bool ModelMetadataHandler::WriteToQuelFile(const std::string_view quelPath, const AssetHandle& handle) {
        using namespace Serialization;

        OsPath metadataPath = quelPath;
        if (!std::filesystem::exists(metadataPath)) {
            std::ofstream file(metadataPath, std::ios::binary);
            if (!file) {
                return false;
            }

            std::string buffer;
            StringQuelWriter writer(buffer);
            writer.Write(SectionEvent{ "Model" });
            writer.CloseSection();
            writer.WriteField("handle", UnquotedString{ handle.ToFormattedString() });

            file.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            return true;
        }

        std::ifstream inFile(metadataPath, std::ios::binary | std::ios::ate);
        if (!inFile) {
            return false;
        }

        const size_t fileSize = inFile.tellg();
        inFile.seekg(0);

        std::string buffer;
        buffer.resize(fileSize);
        inFile.read(buffer.data(), static_cast<std::streamsize>(fileSize));
        inFile.close();

        QuelReader reader(buffer);
        std::string newBuffer;
        StringQuelWriter writer(newBuffer);
        
        std::string_view currentField;
        bool handleUpdated = false;

        for (auto&& parserEvent : reader.Parse()) {
            std::visit([&]<typename TEvent>(const TEvent& e) {
                using T = std::decay_t<TEvent>;

                if constexpr (std::is_same_v<T, SectionEvent>) {
                    writer.Write(e);
                }
                else if constexpr (std::is_same_v<T, FieldEvent>) {
                    currentField = e.Path;
                    writer.Write(e);
                }
                else if constexpr (std::is_same_v<T, ValueEvent>) {
                    if (currentField == "handle" && !handleUpdated) {
                        writer.Write(ValueEvent{ UnquotedString{ handle.ToFormattedString() } });
                        handleUpdated = true;
                    } else {
                        writer.Write(e);
                    }
                }
            }, parserEvent);
        }

        // Write the updated file
        std::ofstream outFile(metadataPath, std::ios::binary);
        if (!outFile) {
            return false;
        }

        outFile.write(newBuffer.data(), static_cast<std::streamsize>(newBuffer.size()));
        return true;
    }
}
