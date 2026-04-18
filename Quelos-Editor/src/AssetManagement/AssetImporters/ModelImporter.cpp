#include "qspch.h"
#include "ModelImporter.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "Quelos/AssetManager/Assets/Model.h"
#include "Quelos/Serialization/Serializer.h"

namespace QuelosEditor {
    using namespace Quelos;

    static glm::vec3 ToGlmVec3(const aiVector3D& vec) {
        return { vec.x, vec.y, vec.z };
    }

    namespace ModelImporter {
        static void SerializeModelMetadata(const Ref<Model>& model, const OsPath& path) {
            using namespace Serialization;

            std::ofstream file(path, std::ios::binary);
            if (!file) {
                QS_CORE_ERROR_TAG(
                    "ModelImporter::SerializeModelMetadata",
                    "Failed to serialize model metadata at {}",
                    path.generic_string()
                );

                return;
            }

            std::string buffer;
            StringQuelWriter writer(buffer);
            writer.Write(SectionEvent { "Model" });
            writer.CloseSection();
            writer.WriteField("handle", UnquotedString { model->GetAssetHandle().ToFormattedString() });

            for (auto& mesh : model->GetMeshes()) {
                writer.Write(SectionEvent { "Mesh" });
                writer.CloseSection();
                writer.WriteField("handle", UnquotedString { mesh->GetAssetHandle().ToFormattedString() });
                writer.WriteField("name", mesh->GetName());
            }

            // TODO: Serialize materials when Model supports them
            // for (auto& material : model->GetMaterials()) {
            //     writer.Write(SectionEvent { "Material" });
            //     writer.CloseSection();
            //     writer.WriteField("handle", UnquotedString { material->GetAssetHandle().ToFormattedString() });
            //     writer.WriteField("name", material->GetName());
            // }

            file.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        }

        static std::optional<ModelMetadata> DeserializeModelMetadata(const OsPath& path) {
            using namespace Serialization;

            if (!std::filesystem::exists(path)) {
                return std::nullopt;
            }

            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file) {
                QS_CORE_ERROR_TAG(
                    "ModelImporter::DeserializeModelMetadata",
                    "Failed to open metadata file '{}'",
                    path.generic_string()
                );

                return std::nullopt;
            }

            const size_t fileSize = file.tellg();
            file.seekg(0);

            std::string assetRegistryBuffer;
            assetRegistryBuffer.resize(fileSize);
            file.read(assetRegistryBuffer.data(), static_cast<std::streamsize>(fileSize));

            QuelReader reader(assetRegistryBuffer);

            ModelMetadata modelMetadata;
            std::string_view currentField;
            MeshMetadata meshMetadata;
            MaterialMetadata materialMetadata;
            std::string currentSection;
            
            for (auto&& parserEvent : reader.Parse()) {
                std::visit([&]<typename TEvent>(const TEvent& e) {
                    using T = std::decay_t<TEvent>;

                    if constexpr (std::is_same_v<T, SectionEvent>) {
                        // Save previous section data
                        if (currentSection == "Mesh" && meshMetadata.Handle) {
                            modelMetadata.MeshesMetadata.push_back(meshMetadata);
                            meshMetadata = {};
                        }
                        else if (currentSection == "Material" && materialMetadata.Handle) {
                            modelMetadata.MaterialsMetadata.push_back(materialMetadata);
                            materialMetadata = {};
                        }
                        currentSection = e.Name;
                    }
                    else if constexpr (std::is_same_v<T, FieldEvent>) {
                        currentField = e.Path;
                    }
                    else if constexpr (std::is_same_v<T, ValueEvent>) {
                        if (const std::string_view* valueResult = std::get_if<std::string_view>(&e.Value)) {
                            if (currentField == "handle") {
                                if (currentSection == "Mesh") {
                                    meshMetadata.Handle = AssetHandle(*valueResult);
                                }
                                else if (currentSection == "Material") {
                                    materialMetadata.Handle = AssetHandle(*valueResult);
                                }
                            }
                            else if (currentField == "name") {
                                if (currentSection == "Mesh") {
                                    meshMetadata.Name = *valueResult;
                                }
                                else if (currentSection == "Material") {
                                    materialMetadata.Name = *valueResult;
                                }
                            }
                        }
                    }
                }, parserEvent);
            }

            // Save final section data
            if (currentSection == "Mesh" && meshMetadata.Handle) {
                modelMetadata.MeshesMetadata.push_back(meshMetadata);
            }
            else if (currentSection == "Material" && materialMetadata.Handle) {
                modelMetadata.MaterialsMetadata.push_back(materialMetadata);
            }

            return modelMetadata;
        }

        bool IsAssetSupported(const std::string_view path) {
            return Extension(path) == ".fbx";
        }

        Ref<Model> ImportModel(const AssetHandle assetHandle, const AssetMetadata& metadata) {
            Assimp::Importer importer;
            const OsPath absolutePath = Project::GetProjectPath() / metadata.FilePath;

            const aiScene* scene = importer.ReadFile(absolutePath.string(),
                                                     aiProcess_CalcTangentSpace |
                                                     aiProcess_Triangulate |
                                                     aiProcess_JoinIdenticalVertices |
                                                     aiProcess_SortByPType);

            if (!scene || !scene->mRootNode) {
                QS_CORE_ERROR_TAG(
                    "MeshImporter::ImportMesh",
                    "Failed to import mesh ({},{}): {}",
                    metadata.FilePath,
                    assetHandle.ToString(),
                    importer.GetErrorString()
                );

                return nullptr;
            }

            ModelMetadata modelMetadata;
            if (const std::optional<ModelMetadata> existingMetadata = DeserializeModelMetadata(absolutePath.string() + ".quel")) {
                modelMetadata = *existingMetadata;
            }

            Ref<Model> model = CreateRef<Model>();
            model->SetAssetHandle(assetHandle);

            Vec<Ref<Mesh>>& meshes = model->GetMeshes();
            meshes.resize(scene->mNumMeshes);

            for (uint32_t i = 0; i < scene->mNumMeshes; i++) {
                const aiMesh* mesh = scene->mMeshes[i];
                if (!mesh) {
                    continue;
                }

                Vec<Vertex> vertices(mesh->mNumVertices);
                for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; vertexIndex++) {
                    auto& vertex = vertices[vertexIndex];
                    vertex.Position = ToGlmVec3(mesh->mVertices[vertexIndex]);

                    if (mesh->HasNormals()) {
                        vertex.Normal = ToGlmVec3(mesh->mNormals[vertexIndex]);
                    }

                    if (mesh->HasTangentsAndBitangents()) {
                        vertex.Tangent = ToGlmVec3(mesh->mTangents[vertexIndex]);
                    }

                    if (mesh->HasTextureCoords(0)) {
                        vertex.UV = { mesh->mTextureCoords[0][vertexIndex].x, mesh->mTextureCoords[0][vertexIndex].y };
                    }
                }

                Vec<uint16_t> indices;
                indices.reserve(mesh->mNumFaces * 3);
                for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {
                    const aiFace& face = mesh->mFaces[faceIndex];
                    for (size_t index = 0; index < face.mNumIndices; index++) {
                        indices.push_back(face.mIndices[index]);
                    }
                }

                const Ref<Mesh> modelMesh = CreateRef<Mesh>(
                    std::move(vertices),
                    std::move(indices),
                    model,
                    mesh->mName.C_Str()
                );

                auto it = std::ranges::find_if(
                    modelMetadata.MeshesMetadata,
                    [&](const MeshMetadata& meta) {
                        return meta.Name == mesh->mName.C_Str();
                    });

                if (it != modelMetadata.MeshesMetadata.end()) {
                    modelMesh->SetAssetHandle(it->Handle);
                    QS_CORE_INFO_TAG(
                        "ModelImporter",
                        "Using existing mesh handle {} for mesh '{}'",
                        it->Handle.ToString(), mesh->mName.C_Str()
                    );
                } else {
                    AssetHandle newHandle = AssetHandle::Generate();
                    modelMesh->SetAssetHandle(newHandle);
                    QS_CORE_INFO_TAG(
                        "ModelImporter",
                        "Generated new mesh handle {} for mesh '{}'",
                        newHandle.ToString(),
                        mesh->mName.C_Str()
                    );
                }

                meshes[i] = modelMesh;
            }

            SerializeModelMetadata(model, absolutePath.string() + ".quel");

            return model;
        }

        std::optional<AssetHandle> ReadAssetHandle(const std::string_view assetPath) {
            using namespace Serialization;

            std::string quelPath = std::string(assetPath) + ".quel";
            
            if (!std::filesystem::exists(quelPath)) {
                return std::nullopt;
            }

            std::ifstream file(quelPath, std::ios::binary | std::ios::ate);
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

        bool WriteAssetHandle(const std::string_view assetPath, const AssetHandle& handle) {
            using namespace Serialization;

            std::string quelPath = std::string(assetPath) + ".quel";

            if (!std::filesystem::exists(quelPath)) {
                std::ofstream file(quelPath, std::ios::binary);
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

            std::ifstream inFile(quelPath, std::ios::binary | std::ios::ate);
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

            std::ofstream outFile(quelPath, std::ios::binary);
            if (!outFile) {
                return false;
            }

            outFile.write(newBuffer.data(), static_cast<std::streamsize>(newBuffer.size()));
            return true;
        }
    }
}
