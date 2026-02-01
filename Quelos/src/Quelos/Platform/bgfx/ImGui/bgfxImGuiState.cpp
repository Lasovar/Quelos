#include "qspch.h"
#include "bgfxImGuiState.h"

#include "imgui_internal.h"

#include "bgfx/embedded_shader.h"

#include "vs_ocornut_imgui.bin.h"
#include "fs_ocornut_imgui.bin.h"
#include "vs_imgui_image.bin.h"
#include "fs_imgui_image.bin.h"

#include "roboto_regular.ttf.h"
#include "robotomono_regular.ttf.h"
#include "icons_kenney.ttf.h"
#include "icons_font_awesome.ttf.h"

#include "icons_kenney.h"
#include "icons_font_awesome.h"
#include "bx/math.h"
#include "glm/gtc/type_ptr.hpp"

#include "Quelos/Core/Application.h"

namespace Quelos {
    static const bgfx::EmbeddedShader s_embeddedShaders[] = {
        BGFX_EMBEDDED_SHADER(vs_ocornut_imgui),
        BGFX_EMBEDDED_SHADER(fs_ocornut_imgui),
        BGFX_EMBEDDED_SHADER(vs_imgui_image),
        BGFX_EMBEDDED_SHADER(fs_imgui_image),

        BGFX_EMBEDDED_SHADER_END()
    };


    struct FontRangeMerge {
        const void* Data;
        size_t Size;
        ImWchar Ranges[3];
    };

    static FontRangeMerge s_FontRangeMerge[] = {
        {s_iconsKenneyTtf, sizeof(s_iconsKenneyTtf), {ICON_MIN_KI, ICON_MAX_KI, 0}},
        {s_iconsFontAwesomeTtf, sizeof(s_iconsFontAwesomeTtf), {ICON_MIN_FA, ICON_MAX_FA, 0}},
    };

#define IMGUI_FLAGS_NONE        UINT8_C(0x00)
#define IMGUI_FLAGS_ALPHA_BLEND UINT8_C(0x01)

    struct TextureBgfx {
        bgfx::TextureHandle handle;
        uint8_t flags;
        uint8_t mip;
        uint32_t unused;
    };

    /// Returns true if both internal transient index and vertex buffer have
    /// enough space.
    ///
    /// @param[in] numVertices Number of vertices.
    /// @param[in] layout Vertex layout.
    /// @param[in] numIndices Number of indices.
    ///
    inline bool checkAvailTransientBuffers(const uint32_t numVertices, const bgfx::VertexLayout& layout,
                                           const uint32_t numIndices) {
        return numVertices == bgfx::getAvailTransientVertexBuffer(numVertices, layout)
            && (numIndices == 0 || numIndices == bgfx::getAvailTransientIndexBuffer(numIndices));
    }


    void bgfxImGuiState::Init(const float fontSize) {
        IMGUI_CHECKVERSION();

        ViewId = 255;

        Context = ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport
        io.ConfigWindowsMoveFromTitleBarOnly = true;

        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures;

        ImGui::StyleColorsDark();

        const bgfx::RendererType::Enum type = bgfx::getRendererType();
        m_Program = bgfx::createProgram(
            bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_ocornut_imgui"),
            bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_ocornut_imgui"),
            true
        );

        u_ImageLodEnabled = bgfx::createUniform("u_imageLodEnabled", bgfx::UniformType::Vec4);
        m_ImageProgram = bgfx::createProgram(
            bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_imgui_image"),
            bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_imgui_image"),
            true
        );

        m_Layout
            .begin()
            .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();

        s_Tex = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

        {
            ImFontConfig config;
            config.FontDataOwnedByAtlas = false;
            config.MergeMode = false;

            const ImWchar* ranges = io.Fonts->GetGlyphRangesDefault();
            m_Font[ImGui::Font::Regular] = io.Fonts->AddFontFromMemoryTTF(
                (void*)s_robotoRegularTtf,
                sizeof(s_robotoRegularTtf),
                fontSize,
                &config,
                ranges
            );

            m_Font[ImGui::Font::Mono] = io.Fonts->AddFontFromMemoryTTF(
                (void*)s_robotoMonoRegularTtf,
                sizeof(s_robotoMonoRegularTtf),
                fontSize - 3.0f,
                &config,
                ranges
            );

            config.MergeMode = true;
            config.DstFont = m_Font[ImGui::Font::Regular];

            for (const auto& frm : s_FontRangeMerge) {
                io.Fonts->AddFontFromMemoryTTF(
                    const_cast<void*>(frm.Data),
                    static_cast<int>(frm.Size),
                    fontSize - 3.0f,
                    &config,
                    frm.Ranges
                );
            }
        }
    }

    void bgfxImGuiState::Destroy() {
        for (ImTextureData* texData : ImGui::GetPlatformIO().Textures) {
            if (1 == texData->RefCount) {
                const auto tex = bx::bitCast<TextureBgfx>(texData->GetTexID());
                bgfx::destroy(tex.handle);
                texData->SetTexID(ImTextureID_Invalid);
                texData->SetStatus(ImTextureStatus_Destroyed);
            }
        }

        ImGui::DestroyContext(Context);

        bgfx::destroy(s_Tex);

        bgfx::destroy(u_ImageLodEnabled);
        bgfx::destroy(m_ImageProgram);
        bgfx::destroy(m_Program);
    }

    void bgfxImGuiState::BeginFrame(uint32_t viewId) {
        ViewId = viewId;

        ImGuiIO& io = ImGui::GetIO();
        io.DeltaTime = Application::Get().GetTime()->DeltaTime();

        ImGui::NewFrame();
    }

    void bgfxImGuiState::EndFrame() {
        ImGui::Render();
        Render(ImGui::GetDrawData());
    }

    void bgfxImGuiState::Render(ImDrawData* drawData) const {
        if (drawData->Textures != nullptr) {
            for (ImTextureData* texData : *drawData->Textures) {
                switch (texData->Status) {
                case ImTextureStatus_WantCreate:
                    {
                        TextureBgfx tex = {
                            .handle = bgfx::createTexture2D(
                                static_cast<uint16_t>(texData->Width),
                                static_cast<uint16_t>(texData->Height),
                                false,
                                1,
                                bgfx::TextureFormat::BGRA8,
                                0
                            ),
                            .flags = IMGUI_FLAGS_ALPHA_BLEND,
                            .mip = 0,
                            .unused = 0,
                        };

                        bgfx::setName(tex.handle, "ImGui Font Atlas");
                        bgfx::updateTexture2D(
                            tex.handle,
                            0, 0, 0, 0,
                            bx::narrowCast<uint16_t>(texData->Width),
                            bx::narrowCast<uint16_t>(texData->Height),
                            bgfx::copy(texData->GetPixels(), texData->GetSizeInBytes())
                        );

                        texData->SetTexID(bx::bitCast<ImTextureID>(tex));
                        texData->SetStatus(ImTextureStatus_OK);
                    }
                    break;

                case ImTextureStatus_WantDestroy:
                    {
                        const auto tex = bx::bitCast<TextureBgfx>(texData->GetTexID());
                        bgfx::destroy(tex.handle);
                        texData->SetTexID(ImTextureID_Invalid);
                        texData->SetStatus(ImTextureStatus_Destroyed);
                    }
                    break;

                case ImTextureStatus_WantUpdates:
                    {
                        const auto tex = bx::bitCast<TextureBgfx>(texData->GetTexID());

                        for (const ImTextureRect& rect : texData->Updates) {
                            const uint32_t bpp = texData->BytesPerPixel;
                            const bgfx::Memory* pix = bgfx::alloc(rect.h * rect.w * bpp);
                            bx::gather(
                                pix->data,
                                texData->GetPixelsAt(rect.x, rect.y),
                                texData->GetPitch(),
                                rect.w * bpp,
                                rect.h
                            );

                            bgfx::updateTexture2D(
                                tex.handle,
                                0, 0, rect.x, rect.y,
                                rect.w,
                                rect.h,
                                pix
                            );
                        }
                    }
                    break;

                default:
                    break;
                }
            }
        }

        // Avoid rendering when minimized
        // scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        const auto dispWidth = static_cast<int32_t>(drawData->DisplaySize.x * drawData->FramebufferScale.x);
        const auto dispHeight = static_cast<int32_t>(drawData->DisplaySize.y * drawData->FramebufferScale.y);
        if (dispWidth <= 0 || dispHeight <= 0) {
            return;
        }

        bgfx::setViewName(ViewId, "ImGui");
        bgfx::setViewMode(ViewId, bgfx::ViewMode::Sequential);


        const bgfx::Caps* caps = bgfx::getCaps();

        {
            glm::mat4 ortho;
            const float x = drawData->DisplayPos.x;
            const float y = drawData->DisplayPos.y;
            const float width = drawData->DisplaySize.x;
            const float height = drawData->DisplaySize.y;

            bx::mtxOrtho(
                glm::value_ptr(ortho),
                x, x + width,
                y + height, y,
                0.0f, 1000.0f,
                0.0f,
                caps->homogeneousDepth
            );

            bgfx::setViewTransform(ViewId, nullptr, glm::value_ptr(ortho));
            bgfx::setViewRect(ViewId, 0, 0, static_cast<uint16_t>(width), static_cast<uint16_t>(height));
        }

        const ImVec2 clipPos = drawData->DisplayPos; // (0,0) unless using multi-viewports
        const ImVec2 clipScale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        // Render command lists
        for (int32_t ii = 0, num = drawData->CmdListsCount; ii < num; ++ii) {
            bgfx::TransientVertexBuffer tvb;
            bgfx::TransientIndexBuffer tib;

            const ImDrawList* drawList = drawData->CmdLists[ii];
            auto numVertices = static_cast<uint32_t>(drawList->VtxBuffer.size());
            auto numIndices = static_cast<uint32_t>(drawList->IdxBuffer.size());

            if (!checkAvailTransientBuffers(numVertices, m_Layout, numIndices)) {
                // not enough space in transient buffer just quit drawing the rest...
                break;
            }

            bgfx::allocTransientVertexBuffer(&tvb, numVertices, m_Layout);
            bgfx::allocTransientIndexBuffer(&tib, numIndices, sizeof(ImDrawIdx) == 4);

            auto verts = reinterpret_cast<ImDrawVert*>(tvb.data);
            bx::memCopy(verts, drawList->VtxBuffer.begin(), numVertices * sizeof(ImDrawVert));

            auto indices = reinterpret_cast<ImDrawIdx*>(tib.data);
            bx::memCopy(indices, drawList->IdxBuffer.begin(), numIndices * sizeof(ImDrawIdx));

            bgfx::Encoder* encoder = bgfx::begin();

            for (const ImDrawCmd *cmd = drawList->CmdBuffer.begin(), *cmdEnd = drawList->CmdBuffer.end(); cmd != cmdEnd;
                 ++cmd) {
                if (cmd->UserCallback) {
                    cmd->UserCallback(drawList, cmd);
                }
                else if (0 != cmd->ElemCount) {
                    uint64_t state = 0
                        | BGFX_STATE_WRITE_RGB
                        | BGFX_STATE_WRITE_A
                        | BGFX_STATE_MSAA;

                    bgfx::TextureHandle th = BGFX_INVALID_HANDLE;
                    bgfx::ProgramHandle program = m_Program;

                    const ImTextureID texId = cmd->GetTexID();

                    if (ImTextureID_Invalid != texId) {
                        const auto tex = bx::bitCast<TextureBgfx>(texId);

                        state |= 0 != (IMGUI_FLAGS_ALPHA_BLEND & tex.flags)
                                     ? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
                                     : BGFX_STATE_NONE;

                        th = tex.handle;

                        if (0 != tex.mip) {
                            const float lodEnabled[4] = {static_cast<float>(tex.mip), 1.0f, 0.0f, 0.0f};
                            bgfx::setUniform(u_ImageLodEnabled, lodEnabled);
                            program = m_ImageProgram;
                        }
                    }
                    else {
                        state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
                    }

                    // Project scissor/clipping rectangles into framebuffer space
                    ImVec4 clipRect;
                    clipRect.x = (cmd->ClipRect.x - clipPos.x) * clipScale.x;
                    clipRect.y = (cmd->ClipRect.y - clipPos.y) * clipScale.y;
                    clipRect.z = (cmd->ClipRect.z - clipPos.x) * clipScale.x;
                    clipRect.w = (cmd->ClipRect.w - clipPos.y) * clipScale.y;

                    if (clipRect.x < dispWidth
                        && clipRect.y < dispHeight
                        && clipRect.z >= 0.0f
                        && clipRect.w >= 0.0f) {
                        const auto xx = static_cast<uint16_t>(bx::max(clipRect.x, 0.0f));
                        const auto yy = static_cast<uint16_t>(bx::max(clipRect.y, 0.0f));
                        encoder->setScissor(
                            xx, yy,
                            static_cast<uint16_t>(bx::min(clipRect.z, 65535.0f) - xx),
                            static_cast<uint16_t>(bx::min(clipRect.w, 65535.0f) - yy)
                        );

                        encoder->setState(state);
                        encoder->setTexture(0, s_Tex, th);
                        encoder->setVertexBuffer(0, &tvb, cmd->VtxOffset, numVertices);
                        encoder->setIndexBuffer(&tib, cmd->IdxOffset, cmd->ElemCount);
                        encoder->submit(ViewId, program);
                    }
                }
            }

            bgfx::end(encoder);
        }
    }
}
