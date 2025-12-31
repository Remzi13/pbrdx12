#include "render/render.h"

#include "render/device/dx12/pipeline_state.h"
#include "render/device/dx12/texture.h"
#include "render/rgraph.h"
#include "render/image.h"
#include "render/utils.h"
#include "render/imgui_system.h"

namespace render {

	namespace {
		UniquePtr<PipelineState> g_scenePiplineState;
		UniquePtr<PipelineState> g_skyBoxPiplineState;
		UniquePtr<PipelineState> g_debugPiplineState;

		UniquePtr<Buffer> g_matBuffer;
		//data::Mesh sphera = data::createMesh("Sphera0.5:5:5");
		//data::Mesh skyBox = data::createMesh("Sphera0.5:10:10");

		void createPipelines( GraphicsDevice* device )
		{
			PipelineStateDescriptor psoDesc;
			auto layout = vector<PipelineStateDescriptor::VertexElementDesc>( {
				{ "POSITION", ResourceFormat::RGB32_FLOAT },
				{ "NORMAL", ResourceFormat::RGB32_FLOAT },
				{ "TEXCOORD", ResourceFormat::RG32_FLOAT },
				{ "TANGENT", ResourceFormat::RGB32_FLOAT },
			} );
			
			psoDesc.setInputLayout( layout );
			psoDesc.setRootSignature( RootSignatureType::COMMON, device );
			psoDesc.setVertexShader( "res\\shaders\\color.hlsl", "VSMain" );
			psoDesc.setPixelShader( "res\\shaders\\color.hlsl", "PSMain" );
			psoDesc.setBlendMode( PipelineStateDescriptor::BlendMode::Alpha );
			psoDesc.setDepth( true, PipelineStateDescriptor::DepthTest::Less );
			psoDesc.setRenderTargetFormat( ResourceFormat::RGBA8_UNORM, ResourceFormat::D24S8, 1 );
			psoDesc.setCullMode( PipelineStateDescriptor::CullMode::None );

			g_scenePiplineState = makeUnique<PipelineStateDx12>( device, psoDesc );
			g_scenePiplineState->init();

			psoDesc.setVertexShader("res\\shaders\\debug.hlsl", "VSMain");
			psoDesc.setPixelShader("res\\shaders\\debug.hlsl", "PSMain");

			g_debugPiplineState = makeUnique<PipelineStateDx12>(device, psoDesc);
			g_debugPiplineState->init();

			// setup sky box psoDsec cullmode none
			psoDesc.setDepth(true, PipelineStateDescriptor::DepthTest::LessEqual);

			psoDesc.setVertexShader("res\\shaders\\skybox.hlsl", "VSMain");
			psoDesc.setPixelShader("res\\shaders\\skybox.hlsl", "PSMain");
			g_skyBoxPiplineState = makeUnique<PipelineStateDx12>(device, psoDesc);
			g_skyBoxPiplineState->init();
		}

		//void drawMesh(CommandContext* context, const data::Mesh& mesh)
		//{
		//	const auto& vertices = mesh.vertices();
		//	const auto& indices = mesh.indices();
		//
		//	const UINT vbByteSize = static_cast<UINT>(vertices.size()) * sizeof(data::Vertex);
		//	const UINT ibByteSize = static_cast<UINT>(indices.size()) * sizeof(std::uint16_t);
		//
		//	device::Allocation vertexData = context->allocate(vbByteSize);
		//	context->setVertexBuffer(Buffer::VertexView(vertexData.Location, vertices.size(), sizeof(data::Vertex), 0));
		//
		//	device::Allocation indexData = context->allocate(ibByteSize);
		//	context->setIndexBuffer(Buffer::IndexView(indexData.Location, indices.size(), 0, ResourceFormat::R16_UINT));
		//
		//	memcpy((char*)vertexData.mappedMemory, vertices.data(), vbByteSize);
		//	memcpy((char*)indexData.mappedMemory, indices.data(), ibByteSize);
		//	context->drawIndexedInstanced(indices.size(), 0, 1, 0, 0);
		//}
	}

	bool Render::init(HWND hwnd, int width, int height, core::SizeI viewportSize)
	{
		bool result = false;
		device_ = createDevice();
		result = device_->init(hwnd);
		
		swapChain_ = createSwapChain(device_.get(), hwnd, width, height, 2, format_);
		
		viewport_ = device_->createTexture( TextureDesc::create2D(viewportSize.x, viewportSize.y, format_, Colors::Green, TextureFlag::ShaderResource | TextureFlag::RenderTarget ), "Viewport" );
		depthStencil_ = device_->createTexture(TextureDesc::create2D(viewportSize.x, viewportSize.y, ResourceFormat::D24S8, {1, 0, 0, 0}, TextureFlag::DepthStencil), "DepthStencil");

		ImGuiSystem::init(this, hwnd);

		return result;
	}

	void Render::resize(int width, int height) const
	{
		if (swapChain_)
			swapChain_->resize(width, height);
	}

	void Render::viewportResize(int width, int height)
	{
		if (width != viewport_->width() || height != viewport_->height())
		{
			viewport_ = device_->createTexture(TextureDesc::create2D(width, height, format_, Colors::Green, TextureFlag::ShaderResource | TextureFlag::RenderTarget), "Viewport");
			depthStencil_ = device_->createTexture(TextureDesc::create2D(width, height, ResourceFormat::D24S8, { 1, 0, 0, 0 }, TextureFlag::DepthStencil), "DepthStencil");
		}
	}

	void Render::createMaterials() const
	{
		//struct MatData
		//{
		//	uint32 diffuseIndex;
		//	uint32 normalIndex;
		//	Vector4 diffuseAlbedo{ 1.0f, 1.0f, 1.0f, 1.0f };
		//	Vector3 fresnelR0{ 0.01f, 0.01f, 0.01f };
		//	float Roughness = .25f;
		//	DirectX::XMFLOAT4X4 gMatTransform;
		//};
		//vector<MatData> data;
		//
		//data::materials().walk([&](const data::Material& mat) {
		//	data.push_back({
		//		textures_.at(mat.diffuseMap)->srvIndex(),
		//		textures_.at(mat.normalMap)->srvIndex(),
		//		mat.diffuseAlbedo,
		//		mat.fresnelR0,
		//		mat.roughness
		//	});
		//	XMStoreFloat4x4(&data.back().gMatTransform, DirectX::XMMatrixTranspose(device::convert(mat.transform)));
		//});
		//
		//if (!data.empty())
		//	g_matBuffer = device_->createBuffer(Buffer::Desc({ data.size() * sizeof(MatData) , sizeof(MatData), BufferFlag::ShaderResource}), "mat_buffer", data.data());
	}	

	void Render::update(float dt)
	{
		ImGuiSystem::update(dt);
	}

	void Render::draw() 
	{
		ImGuiSystem::render(this);
		//struct
		//{
		//	DirectX::XMFLOAT4X4 gViewProj{};
		//	DirectX::XMFLOAT4 gAmbientLight{ 0.25f, 0.25f, 0.35f, 1.0f };
		//	DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
		//	float cbPerObjectPad1 = 0.0f;
		//	// light 
		//	struct Light
		//	{
		//		DirectX::XMFLOAT3 Strength = { 0.5f, 0.5f, 0.5f };
		//		float FalloffStart = 1.0f;							// point/spotlight only
		//		DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };// directional/spotlight only
		//		float FalloffEnd = 10.0f;							// point/spotlight only
		//		DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };  // point/spotlight only
		//		float SpotPower = 64.0f;							// spotlight only
		//	};
		//	Light light[3];
		//	float ltype[3];
		//} perPass;
		//
		////perPass.EyePosW = device::convert(camera.position);
		//
		//auto lights = data::getScene().lights();
		//perPass.gAmbientLight = device::convert(lights.ambientLigh);
		//
		//int i = 0;
		//for (const auto& l : lights.lights)
		//{
		//	perPass.light[i].Strength = device::convert(l.strength);
		//	perPass.light[i].Direction = device::convert(l.direction);
		//	perPass.light[i].Position = device::convert(l.position);
		//	perPass.light[i].FalloffEnd = l.falloffEnd;
		//	perPass.light[i].FalloffStart = l.falloffStart;
		//	perPass.light[i].SpotPower = l.spotPower;
		//	perPass.ltype[i] = l.type;
		//	++i;
		//}
		//XMStoreFloat4x4(&perPass.gViewProj, DirectX::XMMatrixTranspose(device::convert(camera.proj* camera.view)));
		//		
		//RGraph graph;
		//
		//if (true) // sky box
		//{
		//	auto sky = graph.addPass("sky_box");
		//	sky->renderTarget(viewport_.get(), RenderPassColorFlags::Clear);
		//	sky->depthStencil(depthStencil_.get());
		//	auto materialIndex = skyBox_->srvIndex();
		//	sky->bind([materialIndex, perPass](CommandContext* context) {
		//
		//		context->bindRootCBV(BindingSlot::PerPass, &perPass, sizeof(perPass));
		//		if (g_matBuffer)
		//		{
		//			context->bindResources(BindingSlot::SRV, g_matBuffer.get());
		//		}
		//
		//		context->setPipelineState(g_skyBoxPiplineState.get());
		//		struct
		//		{
		//			DirectX::XMFLOAT4X4 gWorld;
		//			DirectX::XMFLOAT4X4 gTexTransform;
		//			uint32 gMaterialIndex;
		//		} perInstance;
		//
		//		perInstance.gMaterialIndex = materialIndex;
		//
		//		auto mTransform = math::utils::transform({ 1.0, 1.0, 1.0 }, { 1.0, 1.0, 1.0 }, { 0, 0, 0 });
		//		XMStoreFloat4x4(&perInstance.gTexTransform, device::convert(mTransform));
		//		XMStoreFloat4x4(&perInstance.gWorld, DirectX::XMMatrixTranspose(DirectX::XMMatrixScaling(5000.0f, 5000.0f, 5000.0f)));
		//
		//		context->bindRootCBV(BindingSlot::PerInstance, &perInstance, sizeof(perInstance));
		//
		//		drawMesh(context, skyBox);
		//
		//		return true; 
		//	});
		//}
		//if (true) // scene
		//{
		//	auto oppaque = graph.addPass("oppaque");
		//	oppaque->renderTarget(viewport_.get(), RenderPassColorFlags::None);
		//	oppaque->depthStencil(depthStencil_.get());
		//	oppaque->bind([](CommandContext* context) {
		//		context->setPipelineState( g_scenePiplineState.get() );
		//	
		//		data::getScene().walk([&](const data::Scene* scene, const data::Scene::Node& node) {
		//			struct
		//			{
		//				DirectX::XMFLOAT4X4 gWorld;
		//				DirectX::XMFLOAT4X4 gTexTransform;
		//				uint32 gMaterialIndex;
		//			} perInstance;
		//
		//			perInstance.gMaterialIndex = data::materials().index(node.materialName());
		//			XMStoreFloat4x4(&perInstance.gTexTransform, DirectX::XMMatrixTranspose(DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f)));
		//			auto mTransform = math::utils::transform(node.pos(), { 1.0, 1.0, 1.0 }, { 0, 0, 0 });
		//			XMStoreFloat4x4(&perInstance.gWorld, XMMatrixTranspose(device::convert(mTransform)));
		//
		//			context->bindRootCBV(BindingSlot::PerInstance, &perInstance, sizeof(perInstance));
		//
		//			drawMesh(context, node.mesh());
		//		});
		//
		//		return true;
		//	});
		//}
		//if (true)
		//{
		//	auto debug = graph.addPass("debug");
		//	debug->renderTarget(viewport_.get(), RenderPassColorFlags::None);
		//	debug->depthStencil(depthStencil_.get());
		//	debug->bind([lights](CommandContext* context) {
		//		context->setPipelineState(g_debugPiplineState.get());
		//
		//		const UINT vbByteSize = static_cast<UINT>(sphera.vertices().size()) * sizeof(data::Vertex);
		//		const UINT ibByteSize = static_cast<UINT>(sphera.indices().size()) * sizeof(std::uint16_t);
		//
		//		device::Allocation vertexData = context->allocate(vbByteSize);
		//		context->setVertexBuffer(Buffer::VertexView(vertexData.Location, sphera.vertices().size(), sizeof(data::Vertex), 0));
		//
		//		device::Allocation indexData = context->allocate(ibByteSize);
		//		context->setIndexBuffer(Buffer::IndexView(indexData.Location, sphera.indices().size(), 0, ResourceFormat::R16_UINT));
		//
		//		memcpy((char*)vertexData.mappedMemory, sphera.vertices().data(), vbByteSize);
		//		memcpy((char*)indexData.mappedMemory, sphera.indices().data(), ibByteSize);
		//
		//		for (const auto& l : lights.lights)
		//		{
		//			struct
		//			{
		//				DirectX::XMFLOAT4X4 gWorld;
		//				DirectX::XMFLOAT3 color{ 0.25f, 0.25f, 0.35f };
		//				float pad;
		//			} perInstance;
		//
		//			perInstance.color = device::convert(l.strength);
		//			XMStoreFloat4x4(&perInstance.gWorld, XMMatrixTranspose(device::convert(math::utils::transform(l.position, { 1.0, 1.0, 1.0 }, { 0, 0, 0 }))));
		//			context->bindRootCBV(BindingSlot::PerInstance, &perInstance, sizeof(perInstance));
		//
		//			context->drawIndexedInstanced(sphera.indices().size(), 0, 1, 0, 0);
		//		}
		//		return false;
		//	});
		//}
		//graph.execute(device_.get());
	}

	void Render::present() const {
		swapChain_->present();

		device_->tick();
	}

	SharedPtr<Texture> Render::createTextureFromFile(const char* fileName) const
	{
		ASSERT(false);
		//Image img;
		//const auto path = settings().get<string>(TEXTURES_PATH) + fileName;
		//img.load(path.c_str());
		//return createTexture(device_.get(), img, fileName);
		return nullptr;
	}

	void Render::loadTexture(const char* file)
	{
		textures_.emplace(file, createTextureFromFile(file));
	}

	vector<string> Render::texturesName() const
	{
		vector<string> names;
		for (const auto& t : textures_)
		{
			names.push_back(t.first);
		}
		return names;
	}

	Texture* Render::backBuffer() const
	{
		return swapChain_->backBuffer(swapChain_->currentBackbuffer());
	}

	void Render::fini()
	{
		g_matBuffer.reset();
		textures_.clear();
		swapChain_.reset();
		device_->fini();
	}
}
