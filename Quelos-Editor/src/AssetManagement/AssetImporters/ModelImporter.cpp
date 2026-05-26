#include "qspch.h"
#include "ModelImporter.h"

#include "magic_enum/magic_enum.hpp"

#include "Quelos/Project/Project.h"
#include "Quelos/AssetManager/Assets/Mesh.h"
#include "Quelos/AssetManager/AssetRegistryExtensions.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "AssetManagement/EditorAssetImporter.h"
#include "Quelos/Utility/FileSystem.h"

namespace QuelosEditor {
    using namespace Quelos;

    namespace ModelImporter {
        static float3 ToFloat3(const aiVector3D& vec) {
            return {vec.x, vec.y, vec.z};
        }

        static void SerializeModelMetadata(const Model& model, const OsPath& path) {
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
            writer.WriteField("handle", UnquotedString { model.GetAssetID().ToFormattedString() });

            for (const auto& mesh : model.GetMeshes()) {
                writer.Write(SectionEvent { "Mesh" });
                writer.CloseSection();
                writer.WriteField("handle", UnquotedString { mesh.AssetId.ToFormattedString() });
                writer.WriteField("name", mesh.Name);
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

        static ModelMetadata DeserializeModelMetadata(const OsPath& path) {
            using namespace Serialization;

            if (!std::filesystem::exists(path)) {
                return {};
            }

            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file) {
                QS_CORE_ERROR_TAG(
                    "ModelImporter::DeserializeModelMetadata",
                    "Failed to open metadata file '{}'",
                    path.generic_string()
                );

                return {};
            }

            const std::streamsize fileSize = file.tellg();
            file.seekg(0);

            std::string assetRegistryBuffer;
            assetRegistryBuffer.resize(fileSize);
            file.read(assetRegistryBuffer.data(), fileSize);

            QuelReader reader(assetRegistryBuffer);
            std::string_view currentSection;
            std::string_view currentField;

            ModelMetadata modelMetadata;
            MeshMetadata meshMetadata;
            MaterialMetadata materialMetadata;

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
                                    meshMetadata.Handle = AssetID(*valueResult);
                                }
                                else if (currentSection == "Material") {
                                    materialMetadata.Handle = AssetID(*valueResult);
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
            const std::string_view extension = FS::Extension(path);
            return extension == ".fbx" || extension == ".obj" || extension == ".gltf" || extension == ".glb";
        }

        bool ImportModel(void* dataSlot, const AssetMetadata& metadata) {
            const OsPath absolutePath = Project::GetProjectPath() / metadata.FilePath;
            
            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(
                absolutePath.string().c_str(),
                aiProcess_CalcTangentSpace |
                aiProcess_Triangulate |
                aiProcess_JoinIdenticalVertices |
                aiProcess_MakeLeftHanded |
                aiProcess_SortByPType
            );

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
                QS_CORE_ERROR_TAG(
                    "MeshImporter::ImportMesh",
                    "Failed to import mesh ({},{}): {}",
                    metadata.FilePath,
                    metadata.Handle.ToString(),
                    importer.GetErrorString()
                );

                return false;
            }

            auto model = new(dataSlot) Model;
            model->SetAssetID(metadata.Handle);

            Deque<MeshData>& meshes = model->GetMeshes();
            meshes.resize(scene->mNumMeshes);

            ModelMetadata modelMetadata = DeserializeModelMetadata(absolutePath.string() + ".quel");

            for (uint32_t i = 0; i < scene->mNumMeshes; i++) {
                const aiMesh* mesh = scene->mMeshes[i];
                if (!mesh) {
                    continue;
                }

                MeshData& modelMesh = meshes[i];
                modelMesh.Name = mesh->mName.C_Str();

                modelMesh.Vertices.resize(mesh->mNumVertices);
                for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; vertexIndex++) {
                    auto& vertex = modelMesh.Vertices[vertexIndex];
                    vertex.Position = ToFloat3(mesh->mVertices[vertexIndex]);

                    if (mesh->HasNormals()) {
                        vertex.Normal = ToFloat3(mesh->mNormals[vertexIndex]);
                    }

                    if (mesh->HasTangentsAndBitangents()) {
                        vertex.Tangent = ToFloat3(mesh->mTangents[vertexIndex]);
                    }

                    if (mesh->HasTextureCoords(0)) {
                        vertex.UV.x = mesh->mTextureCoords[0][vertexIndex].x;
                        vertex.UV.y = mesh->mTextureCoords[0][vertexIndex].y;
                    }
                }

                Vec<uint16_t>& indices = modelMesh.Indices;
                indices.reserve(mesh->mNumFaces * 3);
                for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {
                    const aiFace& face = mesh->mFaces[faceIndex];
                    for (size_t index = 0; index < face.mNumIndices; index++) {
                        indices.push_back(face.mIndices[index]);
                    }
                }

                auto it = std::ranges::find_if(
                    modelMetadata.MeshesMetadata,
                    [&](const MeshMetadata& meta) {
                        return meta.Name == mesh->mName.C_Str();
                    });

                if (it != modelMetadata.MeshesMetadata.end()) {
                    modelMesh.AssetId = it->Handle;
                } else {
                    AssetID newHandle = AssetID::Generate();
                    modelMesh.AssetId = newHandle;
                    QS_CORE_TRACE_TAG(
                        "ModelImporter",
                        "Generated new mesh handle {} for mesh '{}'",
                        newHandle.ToString(),
                        mesh->mName.C_Str()
                    );
                }
            }

            // Save metadata with any new handles
            SerializeModelMetadata(*model, absolutePath.string() + ".quel");

            return model;
        }

        AssetID ReadAssetHandle(std::string_view assetPath) {
            using namespace Serialization;

            const OsPath absolutePath = Project::GetProjectPath() / fmt::format("{}{}", assetPath, ".quel");

            if (!std::filesystem::exists(absolutePath)) {
                return {};
            }

            std::ifstream file(absolutePath, std::ios::binary | std::ios::ate);
            if (!file) {
                return {};
            }

            const size_t fileSize = file.tellg();
            file.seekg(0);

            std::string buffer;
            buffer.resize(fileSize);
            file.read(buffer.data(), static_cast<std::streamsize>(fileSize));

            QuelReader reader(buffer);

            std::string_view currentField;
            AssetID modelHandle;

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
                                modelHandle = AssetID(*valueResult);
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

        bool WriteAssetHandle(std::string_view assetPath, const AssetID& handle) {
            using namespace Serialization;

            const OsPath absolutePath = Project::GetProjectPath() / fmt::format("{}{}", assetPath, ".quel");

            if (!std::filesystem::exists(absolutePath)) {
                std::ofstream file(absolutePath, std::ios::binary);
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

            std::ifstream inFile(absolutePath, std::ios::binary | std::ios::ate);
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

            std::ofstream outFile(absolutePath, std::ios::binary);
            if (!outFile) {
                return false;
            }

            outFile.write(newBuffer.data(), static_cast<std::streamsize>(newBuffer.size()));
            return true;
        }

        Vec<AssetMetadata> RegisterModelSubAssets(
            const std::string_view assetPath,
            const AssetMetadata& modelMetadata
        ) {
            Vec<AssetMetadata> subAssets;

            // Load model metadata to get sub-asset information
            std::string absolutePath = (Project::GetProjectPath() / assetPath).generic_string();
            Model model;
            ImportModel(&model, modelMetadata);

            // Register mesh sub-assets
            for (const auto& meshMeta : model.GetMeshes()) {
                AssetMetadata meshMetadata = {
                    meshMeta.AssetId,
                    fmt::format("{}/Meshes/{}", modelMetadata.FilePath, meshMeta.Name),
                    Mesh::GetStaticType(),
                    modelMetadata.Handle,
                };
                subAssets.push_back(meshMetadata);
            }

            // Register material sub-assets
            /* TODO:
            for (const auto& materialMeta : metadata.MaterialsMetadata) {
                AssetMetadata materialMetadata = {
                    materialMeta.Handle,
                    modelMetadata.FilePath,
                    Material::GetStaticType(),
                    modelMetadata.Handle,
                    materialMeta.Name
                };
                subAssets.push_back(materialMetadata);
            }
            */

            return subAssets;
        }

        bool ResolveMeshSubAsset(
            void* dataSlot,
            const AssetMetadata& meshMetadata
        ) {
            // Load the parent model and extract the mesh
            AssetRef<Model> model(meshMetadata.ParentId);
            if (!model) {
                return false;
            }

            for (MeshData& mesh : model->GetMeshes()) {
                if (mesh.AssetId == meshMetadata.Handle) {
                    new (dataSlot) Mesh(std::move(model), &mesh);
                    return true;
                }
            }

            return false;
        }

        // Asset registry extension implementations
        Vec<AssetMetadata> RegisterAdditionalAssets(
            std::string_view assetPath,
            const AssetMetadata& mainAssetMetadata,
            void* userData
        ) {
            if (mainAssetMetadata.Type != Model::GetStaticType()) {
                return {};
            }

            return RegisterModelSubAssets(assetPath, mainAssetMetadata);
        }

        bool ResolveSubAsset(
            void* dataSlot,
            const AssetMetadata& subAssetMetadata,
            void* userData
        ) {
            if (subAssetMetadata.Type == Mesh::GetStaticType()) {
                return ResolveMeshSubAsset(dataSlot, subAssetMetadata);
            }

            return false;
        }

        bool HandlesAssetType(const AssetType& type, void* userData) {
            return type == Model::GetStaticType() || type == Mesh::GetStaticType()/* || type == Material::GetStaticType()*/;
        }

        // Registration helpers
        EditorAssetImporterConfig GetImporterConfig() {
            return {
                Model::GetStaticType(),
                ImportModel,
                IsAssetSupported,
                ImportModel,
                ReadAssetHandle,
                WriteAssetHandle
            };
        }

        AssetRegistryExtensionFunctions GetRegistryExtensionFunctions() {
            return {
                RegisterAdditionalAssets,
                ResolveSubAsset,
                HandlesAssetType,
                nullptr
            };
        }

        void Initialize() {
            // Register with editor asset importer
            EditorAssetImporter::RegisterAssetImporter(GetImporterConfig());

            // Register asset registry extension
            const auto extensionFunctions = GetRegistryExtensionFunctions();
            AssetRegistryExtensions::RegisterExtension(extensionFunctions);
        }
    }
}
