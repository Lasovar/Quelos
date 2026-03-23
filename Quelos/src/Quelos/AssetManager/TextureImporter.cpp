#include "qspch.h"
#include "TextureImporter.h"

#include "stb_image.h"
#include "Quelos/Core/Buffer.h"
#include "Quelos/Project/Project.h"

namespace Quelos {
    Ref<Texture2D> TextureImporter::ImportTexture2D(const AssetMetadata& metadata) {
        int width, height, channels;

        const Path assetPath = Project::GetAssetsPath() / metadata.FilePath;
        stbi_uc* data = stbi_load(
            assetPath.c_str(),
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
        const Buffer dataBuffer = Buffer::Adopt(data, width * height * channels, stbi_image_free);

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

        return Texture2D::Create(textureSpecs, dataBuffer.GetView());
    }
}
