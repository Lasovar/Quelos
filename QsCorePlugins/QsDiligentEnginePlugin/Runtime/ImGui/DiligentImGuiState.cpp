/*
 *  Copyright 2019-2025 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#include <cstddef>
#include "Quelos/Core/API.h"
#include "imgui.h"
#include "DiligentImGuiState.hpp"
#include "ImGuiDiligentRenderer.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "RefCntAutoPtr.hpp"
#include "BasicMath.hpp"
#include "Renderer/DiligentRendererContext.h"

namespace Diligent
{

ImGuiDiligentCreateInfo::ImGuiDiligentCreateInfo(IRenderDevice* _pDevice,
                                                 TEXTURE_FORMAT _BackBufferFmt,
                                                 TEXTURE_FORMAT _DepthBufferFmt) noexcept :
    pDevice{_pDevice},
    BackBufferFmt{_BackBufferFmt},
    DepthBufferFmt{_DepthBufferFmt}
{}

ImGuiDiligentCreateInfo::ImGuiDiligentCreateInfo(IRenderDevice*       _pDevice,
                                                 const SwapChainDesc& _SCDesc) noexcept :
    ImGuiDiligentCreateInfo{_pDevice, _SCDesc.ColorBufferFormat, _SCDesc.DepthBufferFormat}
{}


DiligentImGuiState::~DiligentImGuiState() {
    m_pRenderer->InvalidateDeviceObjects();
}

void DiligentImGuiState::BeginFrame(uint32_t viewId) {
    const SwapChainDesc desc = Quelos::DiligentRendererContext::Get().GetSwapChain()->GetDesc();
    m_pRenderer->NewFrame(desc.Width, desc.Height, desc.PreTransform);
    ImGui::NewFrame();
}

void DiligentImGuiState::EndFrame() {
    auto* m_pCtx = Quelos::DiligentRendererContext::Get().GetImmediateContext();
    auto* m_pSwapChain = Quelos::DiligentRendererContext::Get().GetSwapChain();

    auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    auto* pDSV = m_pSwapChain->GetDepthBufferDSV();

    // Clear and bind the swapchain targets
    m_pCtx->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    constexpr float clearColor[] = { 0.f, 0.f, 0.f, 1.f };
    m_pCtx->ClearRenderTarget(pRTV, clearColor,
                              RESOURCE_STATE_TRANSITION_MODE_VERIFY);
    m_pCtx->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0,
                              RESOURCE_STATE_TRANSITION_MODE_VERIFY);

    // No need to call ImGui::EndFrame as ImGui::Render calls it automatically
    ImGui::Render();
    m_pRenderer->RenderDrawData(m_pCtx, ImGui::GetDrawData());
}

void DiligentImGuiState::Init() {
    ImGui::CreateContext();

    const auto& context = Quelos::DiligentRendererContext::Get();
    ImGuiDiligentCreateInfo CI(context.GetRenderingDevice(), context.GetSwapChain()->GetDesc());
    m_pRenderer = std::make_unique<ImGuiDiligentRenderer>(CI);
    m_pRenderer->CreateDeviceObjects();
}

void DiligentImGuiState::Destroy() {
    m_pRenderer->InvalidateDeviceObjects();
}
} // namespace Diligent
