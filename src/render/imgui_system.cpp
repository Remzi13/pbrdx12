#include "render/imgui_system.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_internal.h"


#include "core/std_types.h"

#include "render/device/dx12/pipeline_state.h"
#include "render/device/dx12/device.h"


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace render::ImGuiSystem {

	namespace {
		SharedPtr<PipelineState> g_imguiPipelineState;
		SharedPtr<Texture> g_FontTexture;
		GraphicsDevice* g_device;
	}

	void init(render::Render* render, WindowHandle window)
	{
		g_device = render->device();

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
		io.ConfigViewportsNoDefaultParent = true;
		//io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
		//io.BackendRendererUserData = pDevice;

		ImGui::StyleColorsDark();
		ImGui_ImplWin32_Init(window);
		ImFontConfig fontConfig;
		fontConfig.OversampleH = 2;
		fontConfig.OversampleV = 2;
		//io.Fonts->AddFontFromFileTTF("Fonts/NotoSans-Regular.ttf", 20.0f, &fontConfig);
		io.Fonts->AddFontDefault();
		ResourceFormat pixelFormat = ResourceFormat::RGBA8_UNORM;
		unsigned char* pPixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pPixels, &width, &height);

		D3D12_SUBRESOURCE_DATA data;
		data.pData = pPixels;
		data.RowPitch = rowPitch(pixelFormat, width);
		data.SlicePitch = slicePitch(pixelFormat, width, height);
		g_FontTexture = static_cast<DeviceDx12*>(g_device)->createTexture(
			TextureDesc::create2D(width, height, pixelFormat, Colors::Black, TextureFlag::ShaderResource), "ImGui Font", nullptr, { data });

		PipelineStateDescriptor psoDesc;
		auto layout = vector<PipelineStateDescriptor::VertexElementDesc>({
			{ "POSITION", ResourceFormat::RG32_FLOAT },
			{ "TEXCOORD", ResourceFormat::RG32_FLOAT },
			{ "COLOR", ResourceFormat::RGBA8_UNORM },
			});
		psoDesc.setInputLayout(layout);
		psoDesc.setRootSignature(RootSignatureType::COMMON, g_device);
		psoDesc.setVertexShader( "..\\shaders\\imgui.hlsl", "VSMain");
		psoDesc.setPixelShader( "..\\shaders\\imgui.hlsl", "PSMain");
		psoDesc.setBlendMode(PipelineStateDescriptor::BlendMode::Alpha);
		psoDesc.setDepth(false, PipelineStateDescriptor::DepthTest::Always);
		psoDesc.setRenderTargetFormat(ResourceFormat::RGBA8_UNORM, ResourceFormat::Unknown, 1);
		psoDesc.setCullMode(PipelineStateDescriptor::CullMode::None);

		auto pipelineState = makeShared<PipelineStateDx12>(g_device, psoDesc);
		pipelineState->init();
		g_imguiPipelineState = pipelineState;

		//ui::addWindow(memory::makeShared<ui::ViewportWindow>(render));
		//ui::addWindow(memory::makeShared<ui::SceneWindow>());
		//ui::addWindow(memory::makeShared<ui::MaterialWindow>(render));
		//ui::addWindow(memory::makeShared<ui::LightWindow>());
		//ui::addWindow(memory::makeShared<ui::SettingsWindow>());
	}

	bool input(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
		{
			return true;
		}
		//if (ImGuizmo::IsUsing())
		//	return true;
		return false;
	}
	
	void update(float dt)
	{
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		// TODO:ImGuizmo
		//ImGuizmo::BeginFrame();
		
		ImGuiViewport* pViewport = ImGui::GetMainViewport();
		ImGuiID dockspace = ImGui::DockSpaceOverViewport(ImGui::GetID("Dockspace"), pViewport);
				
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("Windows"))
			{
				//const auto& windows = ui::windows();
				//for (const auto& w : windows)
				//{
				//	bool s = w->enable();
				//	if (ImGui::Checkbox(w->name(), &s))
				//	{
				//		w->enable(s);
				//	}
				//}
				ImGui::EndMenu();
			}
			ImGuiIO& io = ImGui::GetIO();
			ImGui::Text("[%.3f/%.1f/%.1f]", dt, 1000.0f / io.Framerate, io.Framerate);
			ImGui::EndMainMenuBar();
		}

		//for (const auto& w : windows)
		//{
		//	w->update(dt);
		//}
	}

	void render(render::Render* render)
	{
		auto context = g_device->getCommandContext(CommandQueue::GRAPHICS);
		
		context->insertResourceBarrier(render->backBuffer(), ResourceState::Present, ResourceState::RenderTarget, 0xffffffff);
		context->begin(CommandContext::RenderPassInfo(render->backBuffer(), nullptr));

		ImGui::Render();

		ImDrawData* drawData = ImGui::GetDrawData();

		struct
		{
			Vector4 ScaleOffset;
			uint32 mTextureIndex;
		} params;

		params.ScaleOffset = Vector4(
			2.0f / drawData->DisplaySize.x,
			-2.0f / drawData->DisplaySize.y,
			-(drawData->DisplayPos.x + drawData->DisplayPos.x + drawData->DisplaySize.x) / drawData->DisplaySize.x,
			(drawData->DisplayPos.y + drawData->DisplayPos.y + drawData->DisplaySize.y) / drawData->DisplaySize.y);

		context->setRootSignature(getRootSignature(RootSignatureType::COMMON, nullptr).get());
		context->setPipelineState(g_imguiPipelineState.get());
		context->setPrimitiveTopology(PrimitiveTopology::TriangleList);
		
		context->setViewport(core::RectF(0.0f, 0.0f, drawData->DisplaySize.x, drawData->DisplaySize.y));

		uint32 vertexOffset = 0;
		device::Allocation vertexData = context->allocate(sizeof(ImDrawVert) * drawData->TotalVtxCount);
		context->setVertexBuffer(Buffer::VertexView(vertexData.Location, drawData->TotalVtxCount, sizeof(ImDrawVert), 0));

		uint32 indexOffset = 0;
		device::Allocation indexData = context->allocate(sizeof(ImDrawIdx) * drawData->TotalIdxCount);
		context->setIndexBuffer(Buffer::IndexView(indexData.Location, drawData->TotalIdxCount, 0, ResourceFormat::R16_UINT));

		ImVec2 clipOff = drawData->DisplayPos;
		for (int cmdList = 0; cmdList < drawData->CmdListsCount; ++cmdList)
		{
			const ImDrawList* pList = drawData->CmdLists[cmdList];

			memcpy((char*)vertexData.mappedMemory + vertexOffset * sizeof(ImDrawVert), pList->VtxBuffer.Data, pList->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy((char*)indexData.mappedMemory + indexOffset * sizeof(ImDrawIdx), pList->IdxBuffer.Data, pList->IdxBuffer.Size * sizeof(ImDrawIdx));

			for (int cmd = 0; cmd < pList->CmdBuffer.Size; ++cmd)
			{
				const ImDrawCmd* pCmd = &pList->CmdBuffer[cmd];
				if (pCmd->UserCallback)
				{
					pCmd->UserCallback(pList, pCmd);
				}
				else
				{
					ImVec2 clip_min(pCmd->ClipRect.x - clipOff.x, pCmd->ClipRect.y - clipOff.y);
					ImVec2 clip_max(pCmd->ClipRect.z - clipOff.x, pCmd->ClipRect.w - clipOff.y);
					if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
						continue;

					if ((int)pCmd->ClipRect.x >= (int)pCmd->ClipRect.z || (int)pCmd->ClipRect.y >= (int)pCmd->ClipRect.w)
						continue;

					Texture* pTexture = (Texture*)pCmd->GetTexID();
					if (!pTexture)
						pTexture = static_cast<Texture*>(g_FontTexture.get());
					
					ASSERT(pTexture->srvIndex() != 0xFFFFFFFF);
					
					params.mTextureIndex = pTexture->srvIndex();

					context->bindRootCBV(BindingSlot::PerInstance, &params, sizeof(params));
					context->setScissorRect(core::RectF(clip_min.x, clip_min.y, clip_max.x, clip_max.y));
					context->drawIndexedInstanced(pCmd->ElemCount, pCmd->IdxOffset + indexOffset, 1, pCmd->VtxOffset + vertexOffset, 0);
				}
			}
			
			vertexOffset += pList->VtxBuffer.Size;
			indexOffset += pList->IdxBuffer.Size;
		}

		context->end();
		context->insertResourceBarrier(render->backBuffer(), ResourceState::RenderTarget, ResourceState::Present, 0xffffffff);
		context->execute();
		g_device->backCommandContext(context);
	}
}