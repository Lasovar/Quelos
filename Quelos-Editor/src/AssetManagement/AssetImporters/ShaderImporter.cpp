#include "ShaderImporter.h"

#include "EditorLayer.h"

#include "Quelos/Utility/FileSystem.h"
#include "Quelos/Utility/QuelosUtil.h"
#include "slang.h"
#include "slang-com-helper.h"
#include "slang-com-ptr.h"
#include "Quelos/Core/Profiling.h"
#include "Quelos/Renderer/ComputeShader.h"

namespace QuelosEditor {
    namespace ShaderImporter {
        static Slang::ComPtr<slang::IGlobalSession> s_GlobalSession;

        static std::string GetMetadataPath(std::string_view assetPath) {
            return fmt::format(
                "{}/{}.qmeta",
                Project::GetProjectPath().generic_string(),
                assetPath
            );
        }

        static OsPath GetCookedShadersPath() {
            return Project::GetCookedAssetsPath() / "Shaders" / magic_enum::enum_name(Renderer::GetRendererAPI());
        }

        static bool SerializeHandle(const OsPath& path, AssetID handle) {
            using namespace Serialization;

            std::ofstream file(path, std::ios::binary);
            if (!file) {
                QS_CORE_ERROR_TAG(
                    "ShaderImporter",
                    "Failed to serialize shader handle at {}",
                    path.generic_string()
                );

                return false;
            }

            std::string buffer;
            StringQuelWriter writer(buffer);
            writer.Write(SectionEvent{"Shader"});
            writer.CloseSection();
            writer.WriteField("assetId", UnquotedString{handle.ToFormattedString()});

            file.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            return true;
        }

        struct ShaderMetadata {
            AssetID AssetId;
            Vec<MaterialPropertySpec> MaterialProperties;
        };

        static bool SerializeShaderMetadata(
            const OsPath& path,
            const ShaderMetadata& shaderMetadata
        ) {
            using namespace Serialization;

            std::ofstream file(path, std::ios::binary);
            if (!file) {
                QS_CORE_ERROR_TAG(
                    "ShaderImporter",
                    "Failed to serialize shader handle at {}",
                    path.generic_string()
                );

                return false;
            }

            std::string buffer;
            StringQuelWriter writer(buffer);
            writer.Write(SectionEvent{"Shader"});
            writer.CloseSection();
            writer.WriteField("assetId", UnquotedString{shaderMetadata.AssetId.ToFormattedString()});
            writer.Write(ComponentEvent { "MaterialProperties" });
            for (const auto& materialProperty : shaderMetadata.MaterialProperties) {
                writer.Write(FieldEvent { materialProperty.Name });
                writer.Write(TupleBeginEvent{});
                writer.Write(ValueEvent{ UnquotedString { magic_enum::enum_name(materialProperty.Type) } });
                writer.Write(ValueEvent { materialProperty.Size });
                writer.Write(ValueEvent { materialProperty.Offset });
                writer.Write(TupleEndEvent{});
            }

            file.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            return true;
        }

        static std::optional<ShaderMetadata> DeserializeShaderMetadata(const OsPath& path) {
            using namespace Serialization;

            if (!std::filesystem::exists(path)) {
                return {};
            }

            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file) {
                QS_CORE_ERROR_TAG(
                    "ShaderImporter",
                    "DeserializeShaderMetadata: Failed to open metadata file '{}'!",
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
            std::string_view currentComponent;
            bool inTuple = false;
            uint32_t tupleIndex = 0;
            MaterialPropertySpec materialProperty;

            ShaderMetadata metadata;

            for (auto&& parserEvent : reader.Parse()) {
                std::visit(
                    Overloaded {
                        [&](const SectionEvent& event) {
                            currentSection = event.Name;
                        },
                        [&](const ComponentEvent& event) {
                            currentComponent = event.Name;
                        },
                        [&](const FieldEvent& event) {
                            currentField = event.Path;

                            if (currentComponent == "MaterialProperties") {
                                materialProperty.Name = currentField;
                            }
                        },
                        [&](const TupleBeginEvent&) {
                            inTuple = true;
                            tupleIndex = 0;
                        }, [&](const TupleEndEvent&) {
                            inTuple = false;
                            metadata.MaterialProperties.push_back(materialProperty);
                            materialProperty = {};
                        },
                        [&](const ValueEvent& event) {
                            if (currentSection != "Shader") {
                                return;
                            }

                            if (currentComponent.empty()) {
                                if (currentField == "assetId") {
                                    metadata.AssetId = AssetID(std::get<std::string_view>(event.Value));
                                }
                            }
                            else if (currentComponent == "MaterialProperties" && inTuple) {
                                if (tupleIndex == 0) {
                                    materialProperty.Type = magic_enum::enum_cast<MaterialPropertyType>(
                                       std::get<std::string_view>(event.Value)
                                   ).value_or(MaterialPropertyType::Unknown);
                                } else if (tupleIndex == 2) {
                                    const auto sizeSv = std::get<std::string_view>(event.Value);

                                    std::from_chars(
                                        sizeSv.data(),
                                        sizeSv.data() + sizeSv.size(),
                                        materialProperty.Size
                                    );
                                } else if (tupleIndex == 1) {
                                    const auto offsetSv = std::get<std::string_view>(event.Value);

                                    std::from_chars(
                                        offsetSv.data(),
                                        offsetSv.data() + offsetSv.size(),
                                        materialProperty.Offset
                                    );
                                }

                                tupleIndex++;
                            }
                        },
                        []([[maybe_unused]] auto& e) {}
                    },
                    parserEvent);
            }

            return metadata;
        }

        static AssetID DeserializeHandle(const OsPath& path) {
            using namespace Serialization;

            if (!std::filesystem::exists(path)) {
                return {};
            }

            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file) {
                QS_CORE_ERROR_TAG(
                    "ShaderImporter",
                    "DeserializeHandle Failed to open metadata file '{}'",
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
            std::string_view currentComponent;

            AssetID assetId;
            for (auto&& parserEvent : reader.Parse()) {
                if (assetId) {
                    break;
                }

                std::visit(Overloaded{
                               [&](const SectionEvent& event) {
                                   currentSection = event.Name;
                               },
                               [&](const FieldEvent& event) {
                                   currentField = event.Path;
                               }, [&](const ComponentEvent& event) {
                                   currentComponent = event.Name;
                               },
                               [&](const ValueEvent& event) {
                                   if (currentSection != "Shader" || !currentComponent.empty()) {
                                       return;
                                   }

                                   if (currentField == "assetId") {
                                       assetId = AssetID(std::get<std::string_view>(event.Value));
                                   }
                               },
                               []([[maybe_unused]] auto& e) {}
                           },
                           parserEvent);
            }

            return assetId;
        }

        static MaterialPropertyType GetMaterialProperty(slang::VariableLayoutReflection* variableLayout) {
            slang::TypeReflection* fieldType = variableLayout->getType();
            switch (fieldType->getKind()) {
            case slang::TypeReflection::Kind::None:
            case slang::TypeReflection::Kind::ConstantBuffer:
            case slang::TypeReflection::Kind::Resource:
            case slang::TypeReflection::Kind::SamplerState:
            case slang::TypeReflection::Kind::TextureBuffer:
            case slang::TypeReflection::Kind::ShaderStorageBuffer:
            case slang::TypeReflection::Kind::ParameterBlock:
            case slang::TypeReflection::Kind::GenericTypeParameter:
            case slang::TypeReflection::Kind::Interface:
            case slang::TypeReflection::Kind::OutputStream:
            case slang::TypeReflection::Kind::Specialized:
            case slang::TypeReflection::Kind::Feedback:
            case slang::TypeReflection::Kind::Pointer:
            case slang::TypeReflection::Kind::DynamicResource:
            case slang::TypeReflection::Kind::MeshOutput:
            case slang::TypeReflection::Kind::Enum:
            case slang::TypeReflection::Kind::Struct:
            case slang::TypeReflection::Kind::Array:
            case slang::TypeReflection::Kind::Matrix:
                return MaterialPropertyType::Unknown;
            case slang::TypeReflection::Kind::Scalar: {
                slang::VariableReflection* variable = variableLayout->getVariable();
                for (uint32_t attribIndex = 0; attribIndex < variable->getUserAttributeCount(); attribIndex++) {
                    const char* attributeName = variable->getUserAttributeByIndex(attribIndex)->getName();
                    if (!strcmp(attributeName, "Texture2D")) {
                        return MaterialPropertyType::Texture2D;
                    }
                }

                return MaterialPropertyType::Float;
            }
            case slang::TypeReflection::Kind::Vector:
                switch (fieldType->getElementCount()) {
                case 1:
                    return MaterialPropertyType::Float;
                case 2:
                    return MaterialPropertyType::Float2;
                case 3:
                    return MaterialPropertyType::Float3;
                case 4: {
                    slang::VariableReflection* variable = variableLayout->getVariable();
                    for (uint32_t attribIndex = 0; attribIndex < variable->getUserAttributeCount(); attribIndex++) {
                        const char* attributeName = variable->getUserAttributeByIndex(attribIndex)->getName();
                        if (!strcmp(attributeName, "Color")) {
                            return MaterialPropertyType::Color;
                        }
                    }

                    return MaterialPropertyType::Float4;
                }
                default:
                    return MaterialPropertyType::Unknown;
                }
            default:
                return MaterialPropertyType::Unknown;
            }
        }

        struct CompiledShaderData {
            ShaderType Type = ShaderType::Unknown;
            std::string EntryPoint;
            Buffer Code;
            int32_t Order = 0;
            Vec<Pair<PipelineOption, PipelineOptionValue>> PipelineOptions;
            Array<uint64_t, 3> ThreadGroupSize = {0, 0, 0};
        };

        struct ShaderCompilationResult {
            HashMap<std::string, SmallVec<CompiledShaderData, 2>> Passes;
            Vec<MaterialPropertySpec> MaterialProperties;
            HashSet<std::string> Variables;
            uint64_t MaterialSize = 0;
        };

        constexpr ShaderType GetShaderTypeFromStage(const SlangStage stage) {
            switch (stage) {
            case SLANG_STAGE_VERTEX: return ShaderType::Vertex;
            case SLANG_STAGE_HULL: return ShaderType::Hull;
            case SLANG_STAGE_DOMAIN: return ShaderType::Domain;
            case SLANG_STAGE_GEOMETRY: return ShaderType::Geometry;
            case SLANG_STAGE_FRAGMENT: return ShaderType::Fragment;
            case SLANG_STAGE_COMPUTE: return ShaderType::Compute;
            case SLANG_STAGE_RAY_GENERATION: return ShaderType::RayGen;
            case SLANG_STAGE_INTERSECTION: return ShaderType::RayIntersection;
            case SLANG_STAGE_ANY_HIT: return ShaderType::RayAnyHit;
            case SLANG_STAGE_CLOSEST_HIT: return ShaderType::RayClosestHit;
            case SLANG_STAGE_MISS: return ShaderType::RayMiss;
            case SLANG_STAGE_CALLABLE: return ShaderType::Callable;
            case SLANG_STAGE_MESH: return ShaderType::Mesh;
            case SLANG_STAGE_AMPLIFICATION: return ShaderType::Amplification;
            case SLANG_STAGE_NONE:
            case SLANG_STAGE_DISPATCH:
            case SLANG_STAGE_COUNT:
            default:
                return ShaderType::Unknown;
            }
        }

        Option<ShaderCompilationResult> CompileShader(const std::string& shaderPath) {
            // TODO: Use AssetID or AssetPath/AssetName
            QS_PROFILE_SCOPED_N("Shader compilation");
            slang::SessionDesc sessionDesc;

            SmallVec<slang::CompilerOptionEntry, 3> compilerOptions;

            compilerOptions.push_back({
                slang::CompilerOptionName::GenerateWholeProgram,
                { slang::CompilerOptionValueKind::Int, 1 }
            });

            slang::TargetDesc targetDesc;
            switch (Renderer::GetRendererAPI()) {
            case RendererAPI::Direct3D11:
            case RendererAPI::Direct3D12: {
                targetDesc.format = SLANG_HLSL;
                targetDesc.profile = s_GlobalSession->findProfile("sm_5_1");
                break;
            }
            case RendererAPI::Vulkan: {
                targetDesc.format = SLANG_SPIRV;
                targetDesc.profile = s_GlobalSession->findProfile("spirv_1_5");

                compilerOptions.push_back({slang::CompilerOptionName::VulkanUseEntryPointName, {slang::CompilerOptionValueKind::Int, 1}});
                compilerOptions.push_back({slang::CompilerOptionName::EmitSpirvDirectly, {slang::CompilerOptionValueKind::Int, 1}});
                break;
            }
            case RendererAPI::OpenGL: {
                targetDesc.format = SLANG_GLSL;
                targetDesc.profile = s_GlobalSession->findProfile("glsl_450");
                break;
            }
            default:
                QS_CORE_ASSERT(false, "Unsupported renderer API");
                break;
            }

            targetDesc.compilerOptionEntries = compilerOptions.data();
            targetDesc.compilerOptionEntryCount = compilerOptions.size();

            sessionDesc.targets = &targetDesc;
            sessionDesc.targetCount = 1;

            std::string parentPath = (Project::GetProjectPath() / shaderPath).parent_path().generic_string();
            const char* searchPaths[] = {parentPath.c_str()};
            sessionDesc.searchPaths = searchPaths;
            sessionDesc.searchPathCount = 1;

            Slang::ComPtr<slang::ISession> session;
            s_GlobalSession->createSession(sessionDesc, session.writeRef());

            const std::string moduleName(FS::Stem(shaderPath));
            Slang::ComPtr<slang::IBlob> diagnostics;
            slang::IModule* module = session->loadModule(moduleName.c_str(), diagnostics.writeRef());
            if (diagnostics) {
                QS_CORE_TRACE_TAG("Slang", "{}", static_cast<const char*>(diagnostics->getBufferPointer()));
                diagnostics = nullptr;
            }

            slang::ProgramLayout* layout = module->getLayout(0, diagnostics.writeRef());
            if (diagnostics) {
                QS_CORE_ERROR_TAG("Slang", "{}", static_cast<const char*>(diagnostics->getBufferPointer()));
                diagnostics = nullptr;
            }

            ShaderCompilationResult result;

            for (uint32_t parameterIndex = 0; parameterIndex < layout->getParameterCount(); parameterIndex++) {
                slang::VariableLayoutReflection* parameter = layout->getParameterByIndex(parameterIndex);
                if (strcmp(parameter->getName(), "Materials") != 0) {
                    continue;
                }

                slang::TypeLayoutReflection* type = parameter->getTypeLayout()->getElementTypeLayout();

                result.MaterialSize = type->getSize();
                for (uint32_t typeFieldIndex = 0; typeFieldIndex < type->getFieldCount(); typeFieldIndex++) {
                    slang::VariableLayoutReflection* field = type->getFieldByIndex(typeFieldIndex);
                    MaterialPropertyType propertyType = GetMaterialProperty(field);
                    const uint64_t fieldSize = field->getTypeLayout()->getSize();

                    result.MaterialProperties.emplace_back(field->getName(), propertyType, field->getOffset(), fieldSize);
                }
            }

            struct ShaderInfo {
                const char* EntryPointName = nullptr;
                Slang::ComPtr<slang::IEntryPoint> EntryPoint;
                int32_t Order = 0;
                SlangStage Stage = SLANG_STAGE_NONE;
                Vec<Pair<PipelineOption, PipelineOptionValue>> PipelineOptions;
                Array<uint64_t, 3> ThreadGroupSize = {0, 0, 0};

                struct Compare {
                    constexpr bool operator()(const ShaderInfo& a, const ShaderInfo& b) const {
                        if (a.Order != b.Order) {
                            return a.Order < b.Order;
                        }

                        return a.Stage < b.Stage;
                    }
                };
            };

            HashMap<std::string, SortedVec<ShaderInfo, ShaderInfo::Compare>> passMap;

            for (int i = 0; i < module->getDefinedEntryPointCount(); i++) {
                ShaderInfo shaderInfo;

                module->getDefinedEntryPoint(i, shaderInfo.EntryPoint.writeRef());

                slang::ProgramLayout* entryPointLayout = shaderInfo.EntryPoint->getLayout();
                slang::EntryPointReflection* entryPointReflection = entryPointLayout->getEntryPointByIndex(0);
                slang::FunctionReflection* function = shaderInfo.EntryPoint->getFunctionReflection();

                shaderInfo.EntryPointName = function->getName();
                shaderInfo.Stage = entryPointReflection->getStage();

                if (shaderInfo.Stage == SLANG_STAGE_COMPUTE) {
                    entryPointReflection->getComputeThreadGroupSize(3, shaderInfo.ThreadGroupSize.data());
                }

                std::string_view passName;

                for (uint32_t attribIndex = 0; attribIndex < function->getUserAttributeCount(); attribIndex++) {
                    slang::UserAttribute* attribute = function->getUserAttributeByIndex(attribIndex);

                    if (strcmp(attribute->getName(), "Pass") == 0) {
                        size_t passNameSize = 0;
                        const char* passNameData = nullptr;

                        passNameData = attribute->getArgumentValueString(0, &passNameSize);

                        if (passNameData) {
                            passName = std::string_view(passNameData, passNameSize);
                        }

                        attribute->getArgumentValueInt(1, &shaderInfo.Order);
                    } else if (strcmp(attribute->getName(), "DepthEnable") == 0) {
                        int32_t value = 0;
                        attribute->getArgumentValueInt(0, &value);
                        shaderInfo.PipelineOptions.emplace_back( PipelineOption::DepthEnable, value );
                    } else if (strcmp(attribute->getName(), "DepthWriteEnable") == 0) {
                        int32_t value = 0;
                        attribute->getArgumentValueInt(0, &value);
                        shaderInfo.PipelineOptions.emplace_back( PipelineOption::DepthWriteEnable, value );
                    } else if (strcmp(attribute->getName(), "CullMode") == 0) {
                        int32_t value = 0;
                        attribute->getArgumentValueInt(0, &value);
                        shaderInfo.PipelineOptions.emplace_back( PipelineOption::CullMode, value );
                    }
                }

                if (passName.empty()) {
                    passName = "GBuffer";
                }

                passMap[std::string(passName)].emplace(shaderInfo);
            }

            if (diagnostics) {
                QS_CORE_ERROR_TAG(
                    "Slang",
                    "{}",
                    static_cast<const char*>(diagnostics->getBufferPointer())
                );

                diagnostics = nullptr;
            }

            Vec<slang::IComponentType*> components;
            components.push_back(module);
            for (const auto& shaders : passMap | std::views::values) {
                std::ranges::transform(
                    shaders,
                    std::back_inserter(components),
                    [](const ShaderInfo& info) {
                        return info.EntryPoint;
                    }
                );
            }

            Slang::ComPtr<slang::IComponentType> program;
            session->createCompositeComponentType(
                components.data(),
                components.size(),
                program.writeRef()
            );

            Slang::ComPtr<slang::IComponentType> linkedProgram;
            program->link(linkedProgram.writeRef(), diagnostics.writeRef());

            slang::ProgramLayout* programLayout = linkedProgram->getLayout();
            for (uint32_t paramIndex = 0; paramIndex < programLayout->getParameterCount(); paramIndex++) {
                slang::VariableLayoutReflection* variable = programLayout->getParameterByIndex(paramIndex);
                result.Variables.emplace(variable->getName());
            }

            for (auto& [passName, shaders] : passMap) {
                for (uint32_t i = 0; i < shaders.size(); i++) {
                    ShaderInfo& shader = shaders[i];

                    Slang::ComPtr<slang::IBlob> blob;
                    linkedProgram->getEntryPointCode(
                        i,
                        0,
                        blob.writeRef(),
                        diagnostics.writeRef()
                    );

                    if (diagnostics) {
                        QS_CORE_ERROR_TAG(
                            "Slang",
                            "Failed to get entry point '{}' for shader '{}' code: {}",
                            shader.EntryPointName,
                            shaderPath,
                            static_cast<const char*>(diagnostics->getBufferPointer())
                        );

                        diagnostics = nullptr;
                    }

                    if (!blob) {
                        continue;
                    }

                    CompiledShaderData shaderData;
                    shaderData.EntryPoint = shader.EntryPointName;
                    shaderData.Type = GetShaderTypeFromStage(shader.Stage);
                    shaderData.Code = Buffer::Copy(blob->getBufferPointer(), blob->getBufferSize());
                    shaderData.Order = shader.Order;
                    shaderData.PipelineOptions = std::move(shader.PipelineOptions);
                    shaderData.ThreadGroupSize = shader.ThreadGroupSize;

                    result.Passes[passName].push_back(std::move(shaderData));
                }
            }

            if (result.Passes.empty()) {
                return None;
            }

            return result;
        }

        bool Import(void* dataSlot, const AssetMetadata& metadata) {
            Option<ShaderMetadata> shaderMetadata = DeserializeShaderMetadata(GetMetadataPath(metadata.FilePath));
            if (!shaderMetadata) {
                return false;
            }

            const OsPath cookedPath = GetCookedShadersPath();
            const std::string handleStr = metadata.Handle.ToString();
            const OsPath cookedShadersPath = cookedPath / handleStr;

            const Buffer buffer = Utility::ReadFile(cookedShadersPath, false);

            GraphicsShaderCreateInfo createInfo;
            createInfo.Name = FS::Stem(metadata.FilePath);

            Serialization::BinaryReader reader(buffer);
            createInfo.MaterialSize = reader.Read<uint64_t>().value_or(0);
            createInfo.MaterialProperties = shaderMetadata->MaterialProperties;

            uint32_t numOfParameters = reader.Read<uint32_t>().value_or(0);
            createInfo.Variables.reserve(numOfParameters);
            for (uint32_t parameterIndex = 0; parameterIndex < numOfParameters; parameterIndex++) {
                createInfo.Variables.emplace_back(reader.ReadString().value_or(""));
            }

            uint32_t numOfPasses = reader.Read<uint32_t>().value_or(0);
            createInfo.Passes.reserve(numOfPasses);
            for (uint32_t passIndex = 0; passIndex < numOfPasses; passIndex++) {
                std::string_view passName = reader.ReadString().value_or("");
                uint32_t numOfShaders = reader.Read<uint32_t>().value_or(0);

                for (uint32_t shaderIndex = 0; shaderIndex < numOfShaders; shaderIndex++) {
                    ShaderData shader;
                    shader.EntryPoint = reader.ReadString().value_or("");
                    shader.Type = reader.Read<ShaderType>().value_or(ShaderType::Unknown);
                    shader.Order = reader.Read<int32_t>().value_or(0);

                    shader.PipelineOptions.resize(reader.Read<uint32_t>().value_or(0));
                    for (auto& option : shader.PipelineOptions) {
                        option.first = reader.Read<PipelineOption>().value_or(PipelineOption::None);
                        if (uint8_t valueIndex = reader.Read<uint8_t>().value_or(0); valueIndex == 0) {
                            option.second = reader.Read<int32_t>().value_or(0);
                        } else {
                            option.second = std::string(reader.ReadString().value_or(""));
                        }
                    }

                    if (shader.Type == ShaderType::Compute) {
                        BufferView threadGroups = reader.ReadBytes(sizeof(uint64_t) * 3);
                        std::memcpy(shader.ThreadGroupSize.data(), threadGroups.data(), sizeof(uint64_t) * 3);
                    }

                    shader.Code = reader.ReadBytesWithSize();

                    createInfo.Passes[std::string(passName)].push_back(shader);
                }
            }

            if (metadata.Type == GraphicsShader::GetStaticType()) {
                auto* shader = new(dataSlot) GraphicsShader(createInfo);
                shader->SetAssetID(metadata.Handle);
            } else if (metadata.Type == ComputeShader::GetStaticType()) {
                ComputeShaderCreateInfo computeShaderCreateInfo;
                computeShaderCreateInfo.Name = FS::Stem(metadata.FilePath);

                for (const auto& shaders : createInfo.Passes | std::views::values) {
                    if (shaders.empty()) {
                        continue;
                    }

                    if (shaders[0].Type != ShaderType::Compute) {
                        continue;
                    }

                    computeShaderCreateInfo.EntryPoint = shaders[0].EntryPoint;
                    computeShaderCreateInfo.Code = shaders[0].Code;
                    for (uint32_t i = 0; i < shaders[0].ThreadGroupSize.size(); i++) {
                        computeShaderCreateInfo.ThreadGroupSize[i] = static_cast<uint32_t>(shaders[0].ThreadGroupSize[i]);
                    }

                    computeShaderCreateInfo.Order = shaders[0].Order;
                    break;
                }

                auto* shader = new(dataSlot) ComputeShader(computeShaderCreateInfo);
                shader->SetAssetID(metadata.Handle);
            }

            return true;
        }

        bool IsAssetSupported(const std::string_view assetPath) {
            std::string_view extension = FS::Extension(assetPath);
            return extension == ".slang";
        }

        bool Cook(const AssetMetadata& metadata) {
            const OsPath cookedPath = GetCookedShadersPath();
            std::string handleStr = metadata.Handle.ToString();
            const OsPath cookedShaderPath = cookedPath / handleStr;

            namespace fs = std::filesystem;

            // Check if cache is valid (exists and newer than source)
            const auto srcTime = fs::last_write_time(Project::GetProjectPath() / metadata.FilePath);
            if (fs::exists(cookedShaderPath) && fs::last_write_time(cookedShaderPath) >= srcTime) {
                return true;
            }

            ShaderMetadata shaderMetadata;
            shaderMetadata.AssetId = metadata.Handle;

            Option<ShaderCompilationResult> compilationResult = CompileShader(metadata.FilePath);

            if (!compilationResult) {
                return false;
            }

            ShaderCompilationResult& compiledShaders = compilationResult.value();

            if (!std::filesystem::exists(cookedShaderPath.parent_path())) {
                std::filesystem::create_directories(cookedShaderPath.parent_path());
            }

            shaderMetadata.MaterialProperties = std::move(compiledShaders.MaterialProperties);

            Vec<byte> buffer;
            Serialization::BinaryWriter writer(buffer);

            writer.Write(compiledShaders.MaterialSize);
            writer.Write(static_cast<uint32_t>(compiledShaders.Variables.size()));
            for (const std::string& variable : compiledShaders.Variables) {
                writer.WriteString(variable);
            }

            writer.Write(static_cast<uint32_t>(compiledShaders.Passes.size()));
            for (const auto& [passName, shaders] : compiledShaders.Passes) {
                writer.WriteString(passName);

                writer.Write(static_cast<uint32_t>(shaders.size()));
                for (const auto& shader : shaders) {
                    writer.WriteString(shader.EntryPoint);
                    writer.Write(shader.Type);
                    writer.Write(static_cast<int32_t>(shader.Order));
                    writer.Write(static_cast<uint32_t>(shader.PipelineOptions.size()));
                    for (const auto& option : shader.PipelineOptions) {
                        writer.Write(option.first);
                        writer.Write(static_cast<uint8_t>(option.second.index()));
                        if (option.second.index() == 0) {
                            writer.Write(static_cast<int32_t>(std::get<int32_t>(option.second)));
                        } else {
                            writer.WriteString(std::get<std::string>(option.second));
                        }
                    }

                    if (shader.Type == ShaderType::Compute) {
                        writer.WriteBytes(std::as_bytes(Span(shader.ThreadGroupSize)));
                    }

                    writer.WriteBytesWithSize(shader.Code);
                }
            }

            Utility::WriteFile(cookedShaderPath, buffer);

            SerializeShaderMetadata(GetMetadataPath(metadata.FilePath), shaderMetadata);

            return true;
        }

        AssetID ReadAssetHandle(const std::string_view assetPath) {
            return DeserializeHandle(GetMetadataPath(assetPath));
        }

        bool WriteAssetHandle(const std::string_view assetPath, const AssetID& handle) {
            const OsPath path = GetMetadataPath(assetPath);
            std::optional<ShaderMetadata> metadata = DeserializeShaderMetadata(path);
            if (!metadata) {
                return false;
            }

            metadata.value().AssetId = handle;
            return SerializeShaderMetadata(path, metadata.value());
        }

        bool HandlesAssetType(const AssetType& type, void* userData) {
            return type == GraphicsShader::GetStaticType();
        }

        EditorAssetImporterConfig GetImporterConfig() {
            return {
                GraphicsShader::GetStaticType(),
                Import,
                IsAssetSupported,
                nullptr,
                ReadAssetHandle,
                WriteAssetHandle,
                Cook
            };
        }

        void Initialize() {
            EditorAssetImporter::RegisterAssetImporter(GetImporterConfig());

            constexpr SlangGlobalSessionDesc desc;
            slang::createGlobalSession(&desc, s_GlobalSession.writeRef());
        }
    }
}
