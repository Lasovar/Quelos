#include "qspch.h"

#include "AssetImporter.h"
#include "stb_image.h"
#include "Quelos/Core/Buffer.h"
#include "Quelos/Project/Project.h"

#include "TextureImporter.h"

namespace Quelos {
    bool TextureImporter::IsAssetSupported(const Path& path) {
        static HashSet<std::string> s_ImportableExtensions = {
            ".png", ".jpeg"
        };

        const std::string normalized = NormalizeExt(path.extension().string());
        return s_ImportableExtensions.find(normalized) != s_ImportableExtensions.end();
    }

    Ref<Texture2D> TextureImporter::ImportTexture2D(AssetHandle assetHandle, const AssetMetadata& metadata) {
        int width, height, channels;

        const Path assetPath = Project::GetProjectPath() / metadata.FilePath;
        stbi_uc* data = stbi_load(
            assetPath.string().c_str(),
            &width,
            &height,
            &channels,
            0
        );

        if (!data) {
            QS_CORE_ERROR_TAG(
                "TextureImporter::ImportTexture2D", "Failed to import texture '{}': {}",
                assetPath.string(),
                stbi_failure_reason()
            );

            return nullptr;
        }

        // SIZE * CHANNELS not sure if it's sufficient
        Buffer dataBuffer = Buffer::Adopt(data, width * height * channels, stbi_image_free);

        TextureSpecification textureSpecs;
        textureSpecs.Width = width;
        textureSpecs.Height = height;

        switch (channels) {
        case 3:
            textureSpecs.Format = ImageFormat::RGB;
            break;
        case 4:
            textureSpecs.Format = ImageFormat::RGBA;
            break;
        default:
            QS_CORE_ERROR_TAG(
                "TextureImporter::ImportTexture2D",
                "Failed to import texture '{}'! unsupported number of channels {}",
                assetPath.string(), channels
            );

            return nullptr;
        }

        Ref<Texture2D> texture = Texture2D::Create(textureSpecs, std::move(dataBuffer));
        if (texture) {
            texture->SetAssetHandle(assetHandle);
        }

        return texture;
    }
}
