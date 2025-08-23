#include "FramebufferD3D11.h"

#include "TextureD3D11.h"
#include "DeviceD3D11.h"

#include "HeadersD3D11.h"

namespace RBX {
	namespace Graphics {
		static ID3D11View* createRenderTargetView(ID3D11Device* device11, Texture::Format format, Texture::Type type, ID3D11Resource* texture, uint32_t cubeIndex, uint32_t mipIndex, uint32_t samples) {
			if (Texture::isFormatDepth(format)) {
				D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
				descDSV.Flags = 0u;
				descDSV.Format = TextureD3D11::getInternalFormat(format);

				switch (type) {
				case Texture::Type_2D: {
					descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
					descDSV.Texture2D.MipSlice = mipIndex;

					break;
				}
				case Texture::Type_2DMS: {
					descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

					break;
				}
				case Texture::Type_2DMS_Array: {
					descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
					descDSV.Texture2DMSArray.ArraySize = 1u;
					descDSV.Texture2DMSArray.FirstArraySlice = cubeIndex;

					break;
				}
				case Texture::Type_2D_Array:
				case Texture::Type_Cube:
				case Texture::Type_Cube_Array: {
					descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
					descDSV.Texture2DArray.ArraySize = 1u;
					descDSV.Texture2DArray.FirstArraySlice = cubeIndex;
					descDSV.Texture2DArray.MipSlice = mipIndex;

					break;
				}
				default: {
					RBXASSERT(false);

					break;
				}
				}

				ID3D11DepthStencilView* depthStencilView = nullptr;
				HRESULT hr = device11->CreateDepthStencilView(texture, &descDSV, &depthStencilView);

				if (FAILED(hr))
					throw RBX::runtime_error("Error creating depth stencil view: %x", hr);

				return depthStencilView;
			}
			else {
				D3D11_RENDER_TARGET_VIEW_DESC descRTV = {};
				descRTV.Format = TextureD3D11::getInternalFormat(format);

				switch (type) {
				case Texture::Type_2D: {
					descRTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
					descRTV.Texture2D.MipSlice = mipIndex;

					break;
				}
				case Texture::Type_2DMS: {
					descRTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;

					break;
				}
				case Texture::Type_2DMS_Array: {
					descRTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
					descRTV.Texture2DMSArray.ArraySize = 1u;
					descRTV.Texture2DMSArray.FirstArraySlice = cubeIndex;

					break;
				}
				case Texture::Type_2D_Array:
				case Texture::Type_Cube:
				case Texture::Type_Cube_Array: {
					descRTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
					descRTV.Texture2DArray.ArraySize = 1u;
					descRTV.Texture2DArray.FirstArraySlice = cubeIndex;
					descRTV.Texture2DArray.MipSlice = mipIndex;

					break;
				}
				default:
					RBXASSERT(false);

					break;
				}

				ID3D11RenderTargetView* renderTargetView = nullptr;
				HRESULT hr = device11->CreateRenderTargetView(texture, &descRTV, &renderTargetView);

				if (FAILED(hr))
					throw RBX::runtime_error("Error creating render target view: %x", hr);

				return renderTargetView;
			}
		}

		static ID3D11Texture2D* createRenderTexture(ID3D11Device* device11, uint32_t width, uint32_t height, uint32_t samples, Texture::Format format) {
			bool isDepth = Texture::isFormatDepth(format);

			D3D11_TEXTURE2D_DESC descDepth = {};
			descDepth.Width = width;
			descDepth.Height = height;
			descDepth.MipLevels = 1u;
			descDepth.ArraySize = 1u;
			descDepth.Format = TextureD3D11::getInternalFormat(format);
			descDepth.SampleDesc.Count = samples;
			descDepth.SampleDesc.Quality = 0u;
			descDepth.Usage = D3D11_USAGE_DEFAULT;
			descDepth.BindFlags = isDepth ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET;
			descDepth.CPUAccessFlags = 0u;
			descDepth.MiscFlags = 0u;

			ID3D11Texture2D* rtTexture = nullptr;

			if (FAILED(device11->CreateTexture2D(&descDepth, nullptr, &rtTexture)))
				throw RBX::runtime_error("Error creating render target texture.");

			return rtTexture;
		}

		RenderbufferD3D11::RenderbufferD3D11(Device* device, const std::shared_ptr<TextureD3D11>& owner, uint32_t cubeIndex, uint32_t mipIndex, uint32_t samples)
			: Renderbuffer(device, owner->getFormat(), Texture::getMipSide(owner->getWidth(), mipIndex), Texture::getMipSide(owner->getHeight(), mipIndex), samples)
			, object(nullptr)
			, owner(owner)
		{
			ID3D11Device* device11 = static_cast<DeviceD3D11*>(device)->getDevice11();

			object = createRenderTargetView(device11, owner->getFormat(), owner->getType(), owner->getObject(), cubeIndex, mipIndex, samples);

			// Destructor does not Release() the object, no need to AddRef()
			texture = owner->getObject();
		}

		RenderbufferD3D11::RenderbufferD3D11(Device* device, Texture::Format format, uint32_t width, uint32_t height, uint32_t samples, ID3D11Texture2D* texture)
			: Renderbuffer(device, format, width, height, samples)
			, object(nullptr)
			, texture(texture)
		{
			ID3D11Device* device11 = static_cast<DeviceD3D11*>(device)->getDevice11();

			if (Texture::isFormatDepth(format)) {
				D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
				descDSV.Format = TextureD3D11::getInternalFormat(format);
				descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

				ID3D11DepthStencilView* depthStencilView = nullptr;
				HRESULT hr = device11->CreateDepthStencilView(texture, &descDSV, &depthStencilView);
				RBXASSERT(SUCCEEDED(hr));

				object = depthStencilView;
			}
			else {
				D3D11_RENDER_TARGET_VIEW_DESC descRTV = {};
				descRTV.Format = TextureD3D11::getInternalFormat(format);
				descRTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

				ID3D11RenderTargetView* renderTargetView = nullptr;
				HRESULT hr = device11->CreateRenderTargetView(texture, &descRTV, &renderTargetView);
				RBXASSERT(SUCCEEDED(hr));

				object = renderTargetView;
			}
		}


		RenderbufferD3D11::RenderbufferD3D11(Device* device, Texture::Format format, uint32_t width, uint32_t height, uint32_t samples)
			: Renderbuffer(device, format, width, height, samples)
		{
			ID3D11Device* device11 = static_cast<DeviceD3D11*>(device)->getDevice11();

			texture = createRenderTexture(device11, width, height, samples, format);
			object = createRenderTargetView(device11, format, Texture::Type_2D, texture, 0u, 0u, samples);
		}

		RenderbufferD3D11::~RenderbufferD3D11() {
			ReleaseCheck(object);
			if (!owner)
				ReleaseCheck(texture);
		}

		FramebufferD3D11::FramebufferD3D11(Device* device, const std::vector<std::shared_ptr<Renderbuffer>>& color, const std::shared_ptr<Renderbuffer>& depth)
			: Framebuffer(device, 0u, 0u, 0u)
			, color(color)
			, depth(depth)
		{
			RBXASSERT(!color.empty());

			if (color.size() > device->getCaps().maxDrawBuffers)
				throw RBX::runtime_error("Unsupported framebuffer configuration: too many buffers (%d)", color.size());

			for (size_t i = 0u; i < color.size(); ++i) {
				RenderbufferD3D11* buffer = static_cast<RenderbufferD3D11*>(color[i].get());

				if (i == 0u) {
					width = buffer->getWidth();
					height = buffer->getHeight();
					samples = buffer->getSamples();
				}
				else {
					if (width != buffer->getWidth() || height != buffer->getHeight())
						throw RBX::runtime_error("Unsupported framebuffer configuration: Mismatched color resolution.");

					if (samples != buffer->getSamples())
						throw RBX::runtime_error("Unsupported framebuffer configuration: Mismatched color samples.");
				}
			}

			if (depth) {
				RenderbufferD3D11* buffer = static_cast<RenderbufferD3D11*>(depth.get());

				if (width != buffer->getWidth() || height != buffer->getHeight())
					throw RBX::runtime_error("Unsupported framebuffer configuration: Mismatched depth resolution.");

				if (samples != buffer->getSamples())
					throw RBX::runtime_error("Unsupported framebuffer configuration: Mismatched depth samples.");
			}
		}

		void FramebufferD3D11::download(void* data, size_t size) {
			RBXASSERT(size == width * height * 4u);

			DeviceD3D11* device11 = static_cast<DeviceD3D11*>(device);
			ID3D11DeviceContext* context11 = device11->getImmediateContext11();

			D3D11_TEXTURE2D_DESC desc = {};
			desc.Width = width;
			desc.Height = height;
			desc.MipLevels = 1u;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.SampleDesc.Count = 1u;
			desc.SampleDesc.Quality = 0u;
			desc.ArraySize = 1u;
			desc.Usage = D3D11_USAGE_STAGING;
			desc.BindFlags = 0u;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
			desc.MiscFlags = 0u;

			ID3D11Texture2D* tempTex = nullptr;
			HRESULT hr = device11->getDevice11()->CreateTexture2D(&desc, nullptr, reinterpret_cast<ID3D11Texture2D**>(&tempTex));
			if (FAILED(hr))
				throw RBX::runtime_error("Download frame buffer cant create temp texture %x", hr);

			// get resource bound to view
			ID3D11Resource* resource;
			RenderbufferD3D11* color0 = static_cast<RenderbufferD3D11*>(color[0].get());
			color0->getObject()->GetResource(&resource);
			RBXASSERT(color0->getFormat() == Texture::Format_RGBA16f);

			// copy resource to tex
			context11->CopyResource(tempTex, resource);

			// copy texture to provided memory
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			hr = context11->Map(tempTex, 0u, D3D11_MAP_READ, 0u, &mappedResource);
			RBXASSERT(SUCCEEDED(hr));

			for (unsigned int y = 0u; y < height; ++y) {
				char* dataRow = static_cast<char*>(data) + y * width * 4u;
				memcpy(dataRow, static_cast<char*>(mappedResource.pData) + y * mappedResource.RowPitch, width * 4u);
			}

			// release all the stuff
			context11->Unmap(tempTex, 0u);
			resource->Release();
			ReleaseCheck(tempTex);
		}

		FramebufferD3D11::~FramebufferD3D11()
		{
		}
	}
}
