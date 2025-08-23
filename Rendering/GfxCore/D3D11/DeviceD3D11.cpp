
#include "DeviceD3D11.h"

#include "GeometryD3D11.h"
#include "ShaderD3D11.h"
#include "TextureD3D11.h"
#include "FramebufferD3D11.h"

#include "rbx/rbxTime.h"
#include "rbx/Profiler.h"

#include "StringConv.h"

#include "HeadersD3D11.h"

LOGGROUP(Graphics)
LOGGROUP(VR)

FASTFLAGVARIABLE(DebugD3D11DebugMode, false)

namespace RBX {
	namespace Graphics {
		static uint32_t getMaxSamplesSupported(ID3D11Device* device11) {
			uint32_t result = 1u;

			for (uint32_t mode = 2u; mode <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; mode *= 2u) {
				uint32_t maxQualityLevel;

				HRESULT hr = device11->CheckMultisampleQualityLevels(DXGI_FORMAT_R16G16B16A16_FLOAT, mode, &maxQualityLevel);
				RBXASSERT(SUCCEEDED(hr));
				if (maxQualityLevel <= 0u)
					break;

				result = mode;
			}

			return result;
		}

		static DXGI_ADAPTER_DESC getAdapterDesc(IDXGIDevice* device) {
			IDXGIAdapter* adapter = nullptr;
			device->GetAdapter(&adapter);

			DXGI_ADAPTER_DESC desc = {};
			adapter->GetDesc(&desc);

			adapter->Release();

			return desc;
		}

		template <typename T, typename P> T* queryInterface(P* object) {
			void* result = 0;
			if (SUCCEEDED(object->QueryInterface(__uuidof(T), &result)))
				return static_cast<T*>(result);

			return 0;
		}

		DeviceD3D11::DeviceD3D11(void* windowHandle)
			: windowHandle(windowHandle)
			, device11(nullptr)
			, swapChain11(nullptr)
			, immediateContext(nullptr)
			, frameTimeQueryIssued(false)
			, gpuTime(0.0f)
			, beginQuery(nullptr)
			, endQuery(nullptr)
			, disjointQuery(nullptr)
			, vrEnabled(true)
		{
			createDevice();

			caps = DeviceCaps();

			caps.supportsFramebuffer = true;
			caps.supportsShaders = true;
			caps.supportsFFP = false;
			caps.supportsStencil = true;
			caps.supportsIndex32 = true;

			caps.supportsTextureDXT = true;
			caps.supportsTexturePVR = false;
			caps.supportsTextureHalfFloat = true; //DXGI_FORMAT_R16G16B16A16_FLOAT is always supported on DX11 HW (including featureLvl 10 - 9.3)
			caps.supportsTexture3D = true;
			caps.supportsTextureNPOT = true;
			caps.supportsTextureETC1 = false;

			caps.supportsTexturePartialMipChain = true;

			caps.maxDrawBuffers = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
			caps.maxSamples = getMaxSamplesSupported(device11);

			caps.max1DTextureSize = D3D11_REQ_TEXTURE1D_U_DIMENSION;
			caps.max1DArraySize = D3D11_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION;

			caps.max2DTextureSize = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
			caps.max2DArraySize = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;

			caps.max3DTextureSize = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;

			caps.maxCubeTextureSize = D3D11_REQ_TEXTURECUBE_DIMENSION;

			caps.maxTextureUnits = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
			caps.maxSamplerSlots = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;

			caps.colorOrderBGR = false;
			caps.needsHalfPixelOffset = false;
			caps.requiresRenderTargetFlipping = false;

			caps.retina = false;

			std::pair<uint32_t, uint32_t> dimensions = getFramebufferSize();
			createMainFramebuffer(dimensions.first, dimensions.second);

			// queries
			HRESULT hr;

			D3D11_QUERY_DESC queryDesc = {};
			queryDesc.MiscFlags = 0u;

			queryDesc.Query = D3D11_QUERY_TIMESTAMP;
			hr = device11->CreateQuery(&queryDesc, &beginQuery);
			RBXASSERT(SUCCEEDED(hr));
			hr = device11->CreateQuery(&queryDesc, &endQuery);
			RBXASSERT(SUCCEEDED(hr));

			queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
			hr = device11->CreateQuery(&queryDesc, &disjointQuery);
			RBXASSERT(SUCCEEDED(hr));

			if (IDXGIDevice1* deviceDXGI1 = queryInterface<IDXGIDevice1>(device11)) {
				deviceDXGI1->SetMaximumFrameLatency(1u);
				deviceDXGI1->Release();
			}

			if (IDXGIDevice* deviceDXGI = queryInterface<IDXGIDevice>(device11)) {
				DXGI_ADAPTER_DESC desc = getAdapterDesc(deviceDXGI);

				FASTLOGS(FLog::Graphics, "D3D11 Adapter: %s", utf8_encode(desc.Description));
				FASTLOG2(FLog::Graphics, "D3D11 Adapter: Vendor %04x Device %04x", desc.VendorId, desc.DeviceId);

				// Do not use GPU profiling on Intel cards to avoid a crash in Intel driver when releasing resources in gpuShutdown
				// Also, this vendor id is SO COOL!
				if (desc.VendorId != 0x8086)
					Profiler::gpuInit(getImmediateContext11());

				deviceDXGI->Release();
			}

			if (vr) {
				try {
					vr->setup(this);
				}
				catch (RBX::base_exception& e) {
					FASTLOGS(FLog::VR, "VR ERROR during setup: %s", e.what());
					vr.reset();
				}
			}
		}

		void DeviceD3D11::createMainFramebuffer(uint32_t width, uint32_t height) {
			RBXASSERT(!mainFramebuffer);

			// Get back buffer, create view
			ID3D11Texture2D* backBuffer = nullptr;
			HRESULT hr = swapChain11->GetBuffer(0u, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
			RBXASSERT(SUCCEEDED(hr));

			std::shared_ptr<Renderbuffer> backBufferRB = std::shared_ptr<Renderbuffer>(new RenderbufferD3D11(this, Texture::Format_RGBA8, width, height, 1u, backBuffer));
			std::vector<std::shared_ptr<Renderbuffer>> colorBuffers;
			colorBuffers.push_back(backBufferRB);

			// create frame buffer
			mainFramebuffer.reset(new FramebufferD3D11(this, colorBuffers, nullptr));
		}

		DeviceD3D11::~DeviceD3D11() {
			Profiler::gpuShutdown();

			// we want to release all the DX references before releasing device to be able to test if nothing is hanging
			vr.reset();
			immediateContext.reset();
			mainFramebuffer.reset();

			ReleaseCheck(beginQuery);
			ReleaseCheck(endQuery);
			ReleaseCheck(disjointQuery);

			ReleaseCheck(swapChain11);
			ReleaseCheck(device11);
		}

		void DeviceD3D11::defineGlobalConstants(size_t dataSize) {
			// Since constants are directly set to register values, we impose additional restrictions on constant data
			// The struct should be an integer number of float4 registers, and every constant has to be aligned to float4 boundary
			if (dataSize % 16u != 0u)
				throw std::runtime_error("Global constants buffer is not 16 byte aligned.");

			immediateContext->defineGlobalConstants(dataSize);
		}

		void DeviceD3D11::defineGlobalProcessingData(size_t dataSize) {
			if (dataSize % 16u != 0u)
				throw std::runtime_error("Global processing buffer is not 16 byte aligned.");

			immediateContext->defineGlobalProcessingData(dataSize);
		}

		void DeviceD3D11::defineGlobalMaterialData(size_t dataSize) {
			if (dataSize % 16u != 0u)
				throw std::runtime_error("Global materials buffer is not 16 byte aligned.");

			immediateContext->defineGlobalMaterialData(dataSize);
		}

		void DeviceD3D11::defineInstancedModels(size_t dataSize, size_t elementSize) {
			if (dataSize % 16u != 0u)
				throw std::runtime_error("Instanced model matrix buffer is not 16 byte aligned.");

			immediateContext->defineInstancedModels(dataSize, elementSize);
		}

		void DeviceD3D11::defineGlobalLightList(size_t dataSize, size_t elementSize) {
			if (dataSize % 16u != 0u)
				throw std::runtime_error("Global light list buffer is not 16 byte aligned.");

			immediateContext->defineGlobalLightList(dataSize, elementSize);
		}

		DeviceContext* DeviceD3D11::beginFrame() {
			std::pair<uint32_t, uint32_t> dimensions = getFramebufferSize();

			// Don't render anything if there is no main framebuffer or device is lost
			if (!mainFramebuffer)
				return false;

			// Don't render anything if window size changed; wait for validate
			if (dimensions.first != mainFramebuffer->getWidth() || dimensions.second != mainFramebuffer->getHeight())
				return nullptr;

			//immediateContext->bindFramebuffer(mainFramebuffer.get());
			immediateContext->clearStates();

			if (disjointQuery && beginQuery && endQuery) {
				if (!frameTimeQueryIssued) {
					immediateContext->beginQuery(disjointQuery);
					immediateContext->endQuery(beginQuery);
				}
			}

			return immediateContext.get();
		}

		bool DeviceD3D11::validate() {
			std::pair<uint32_t, uint32_t> dimensions = getFramebufferSize();

			// Don't change anything if window is minimized (getFramebufferSize returns 1x1)
			if (dimensions.first <= 1u && dimensions.second <= 1u)
				return false;

			// Reset device if window size changed
			if (mainFramebuffer && (dimensions.first != mainFramebuffer->getWidth() || dimensions.second != mainFramebuffer->getHeight())) {
				immediateContext->getContextDX11()->OMSetRenderTargets(0u, nullptr, nullptr);

				mainFramebuffer.reset();
				resizeSwapchain();
				createMainFramebuffer(dimensions.first, dimensions.second);
			}

			return true;
		}

		void DeviceD3D11::endFrame() {
			if (disjointQuery && beginQuery && endQuery) {
				if (!frameTimeQueryIssued) {
					immediateContext->endQuery(endQuery);
					immediateContext->endQuery(disjointQuery);
					frameTimeQueryIssued = true;
				}
				else {
					UINT64 tsBeginFrame, tsEndFrame;
					D3D11_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;

					bool queryFinished = immediateContext->getQueryData(disjointQuery, &tsDisjoint, sizeof(tsDisjoint));
					queryFinished &= immediateContext->getQueryData(beginQuery, &tsBeginFrame, sizeof(UINT64));
					queryFinished &= immediateContext->getQueryData(endQuery, &tsEndFrame, sizeof(UINT64));

					if (queryFinished) {
						if (!tsDisjoint.Disjoint)
							gpuTime = (float)(double(tsEndFrame - tsBeginFrame) / double(tsDisjoint.Frequency) * 1000.0);

						frameTimeQueryIssued = false;
					}
				}
			}

			if (vr && vrEnabled) {
				vr->submitFrame(immediateContext.get());
			}

			present();
		}

		DeviceStats DeviceD3D11::getStatistics() const {
			DeviceStats result = {};

			result.gpuFrameTime = gpuTime;

			return result;
		}

		std::shared_ptr<Texture> DeviceD3D11::createTexture(Texture::Type type, Texture::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, Texture::Usage usage) {
			return std::shared_ptr<Texture>(new TextureD3D11(this, type, format, width, height, depth, mipLevels, usage));
		}

		std::shared_ptr<VertexBuffer> DeviceD3D11::createVertexBuffer(size_t elementSize, size_t elementCount, GeometryBuffer::Usage usage) {
			return std::shared_ptr<VertexBuffer>(new VertexBufferD3D11(this, elementSize, elementCount, usage));
		}

		std::shared_ptr<IndexBuffer> DeviceD3D11::createIndexBuffer(size_t elementSize, size_t elementCount, GeometryBuffer::Usage usage) {
			return std::shared_ptr<IndexBuffer>(new IndexBufferD3D11(this, elementSize, elementCount, usage));
		}

		std::shared_ptr<VertexLayout> DeviceD3D11::createVertexLayout(const std::vector<VertexLayout::Element>& elements) {
			return std::shared_ptr<VertexLayout>(new VertexLayoutD3D11(this, elements));
		}

		std::shared_ptr<Geometry> DeviceD3D11::createGeometryImpl(const std::shared_ptr<VertexLayout>& layout, const std::shared_ptr<VertexBuffer>& vertexBuffers, const std::shared_ptr<IndexBuffer>& indexBuffer, uint32_t baseVertexIndex) {
			return std::shared_ptr<Geometry>(new GeometryD3D11(this, layout, vertexBuffers, indexBuffer));
		}

		std::shared_ptr<Framebuffer> DeviceD3D11::createFramebufferImpl(const std::vector<std::shared_ptr<Renderbuffer>>& color, const std::shared_ptr<Renderbuffer>& depth) {
			return std::shared_ptr<Framebuffer>(new FramebufferD3D11(this, color, depth));
		}

		Framebuffer* DeviceD3D11::getMainFramebuffer() {
			return mainFramebuffer.get();
		}

		DeviceVR* DeviceD3D11::getVR() {
			return (vr && vrEnabled) ? vr.get() : nullptr;
		}

		void DeviceD3D11::setVR(bool enabled) {
			vrEnabled = enabled;
		}

		// @deprecated Use a Colorbuffer/Depthbuffer texture and getRenderbuffer instead.
		std::shared_ptr<Renderbuffer> DeviceD3D11::createRenderbuffer(Texture::Format format, uint32_t width, uint32_t height, uint32_t samples) {
			return std::shared_ptr<Renderbuffer>(new RenderbufferD3D11(this, format, width, height, samples));
		}

		std::string DeviceD3D11::createShaderSource(const std::string& path, const std::string& defines, boost::function<std::string(const std::string&)> fileCallback) {
			std::string dx11Defines = defines;
			dx11Defines += " DX11";

			return ShaderProgramD3D11::createShaderSource(path, dx11Defines, this, fileCallback);
		}

		std::vector<char> DeviceD3D11::createShaderBytecode(const std::string& source, const std::string& target, const std::string& entrypoint) {
			return ShaderProgramD3D11::createShaderBytecode(source, target, this, entrypoint);
		}

		shared_ptr<VertexShader> DeviceD3D11::createVertexShader(const std::vector<char>& bytecode) {
			return shared_ptr<VertexShader>(new VertexShaderD3D11(this, bytecode));
		}

		shared_ptr<FragmentShader> DeviceD3D11::createFragmentShader(const std::vector<char>& bytecode) {
			return shared_ptr<FragmentShader>(new FragmentShaderD3D11(this, bytecode));
		}

		shared_ptr<ComputeShader> DeviceD3D11::createComputeShader(const std::vector<char>& bytecode) {
			return shared_ptr<ComputeShader>(new ComputeShaderD3D11(this, bytecode));
		}

		shared_ptr<GeometryShader> DeviceD3D11::createGeometryShader(const std::vector<char>& bytecode) {
			return shared_ptr<GeometryShader>(new GeometryShaderD3D11(this, bytecode));
		}

		shared_ptr<ShaderProgram> DeviceD3D11::createShaderProgram(const shared_ptr<VertexShader>& vertexShader, const shared_ptr<GeometryShader>& geometryShader, const shared_ptr<FragmentShader>& fragmentShader) {
			return shared_ptr<ShaderProgram>(new ShaderProgramD3D11(this, vertexShader, geometryShader, fragmentShader));
		}

		shared_ptr<ShaderProgram> DeviceD3D11::createShaderProgram(const shared_ptr<VertexShader>& vertexShader, const shared_ptr<FragmentShader>& fragmentShader) {
			return shared_ptr<ShaderProgram>(new ShaderProgramD3D11(this, vertexShader, fragmentShader));
		}

		shared_ptr<ShaderProgram> DeviceD3D11::createShaderProgram(const shared_ptr<ComputeShader>& computeShader) {
			return shared_ptr<ShaderProgram>(new ShaderProgramD3D11(this, computeShader));
		}

		shared_ptr<ShaderProgram> DeviceD3D11::createShaderProgramFFP() {
			throw RBX::runtime_error("No Fixed-Function Pipeline (FFP) support");
		}

	}
}
