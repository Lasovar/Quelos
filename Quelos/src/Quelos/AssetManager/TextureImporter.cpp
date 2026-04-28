#include "qspch.h"

#include "stb_image.h"
#include "Quelos/Core/Buffer.h"
#include "Quelos/Project/Project.h"

#include "TextureImporter.h"

namespace Quelos {
    bool TextureImporter::IsAssetSupported(const std::string_view path) {
        static HashSet<std::string_view> s_ImportableExtensions = {
            ".png", ".jpeg"
        };

        return s_ImportableExtensions.find(FS::Extension(path)) != s_ImportableExtensions.end();
    }

    bool TextureImporter::ImportTexture2D(void* dataSlot, const AssetMetadata& metadata) {
        int width, height, channels;

        const OsPath assetPath = Project::GetProjectPath() / metadata.FilePath;
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

            return false;
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

            return false;
        }

        auto* texture = new(dataSlot) Texture2D(textureSpecs, std::move(dataBuffer));
        texture->SetAssetID(metadata.Handle);

        return true;
    }
}
