#include "TextureD3D11.h"

#include "FramebufferD3D11.h"
#include "DeviceD3D11.h"
#include "HeadersD3D11.h"

#include "../../App/include/util/standardout.h"

LOGGROUP(Graphics)

namespace RBX {
	namespace Graphics {
		static const std::array<DXGI_FORMAT, Texture::Format_Count> gTextureFormatD3D11 = {
			DXGI_FORMAT_R8_UNORM,
			DXGI_FORMAT_R8G8_UNORM,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			DXGI_FORMAT_B5G5R5A1_UNORM,
			DXGI_FORMAT_R11G11B10_FLOAT,
			DXGI_FORMAT_R16G16_UNORM,
			DXGI_FORMAT_R16G16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT,

			DXGI_FORMAT_BC1_UNORM,
			DXGI_FORMAT_BC1_UNORM_SRGB,
			DXGI_FORMAT_BC2_UNORM,
			DXGI_FORMAT_BC2_UNORM_SRGB,
			DXGI_FORMAT_BC3_UNORM,
			DXGI_FORMAT_BC3_UNORM_SRGB,
			DXGI_FORMAT_BC4_UNORM,
			DXGI_FORMAT_BC5_UNORM,
			DXGI_FORMAT_BC6H_UF16,
			DXGI_FORMAT_BC7_UNORM,
			DXGI_FORMAT_BC7_UNORM_SRGB,

			DXGI_FORMAT_D16_UNORM,
			DXGI_FORMAT_D24_UNORM_S8_UINT,
			DXGI_FORMAT_D32_FLOAT,
			DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
		};

		static const std::array<DXGI_FORMAT, Texture::Format_Count> gTextureFormatDepthDescD3D11 = {
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,

			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,

			DXGI_FORMAT_R16_TYPELESS,
			DXGI_FORMAT_R24G8_TYPELESS,
			DXGI_FORMAT_R32_TYPELESS,
			DXGI_FORMAT_R32G8X24_TYPELESS,
		};

		static const std::array<DXGI_FORMAT, Texture::Format_Count> gTextureFormatDepthViewD3D11 = {
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,

			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,
			DXGI_FORMAT_UNKNOWN,

			DXGI_FORMAT_R16_UNORM,
			DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
			DXGI_FORMAT_R32_FLOAT,
			DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,
		};

		struct TextureUsageD3D11 {
			D3D11_USAGE usage;
			uint32_t cpuAccess;
			uint32_t bindFlags;
			uint32_t misc;
		};

		static const std::array<TextureUsageD3D11, Texture::Usage_Count> gTextureUsageD3D11 = { {
			{ D3D11_USAGE_DEFAULT, 0u, D3D11_BIND_SHADER_RESOURCE, 0u },														   // Static
			{ D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, D3D11_BIND_SHADER_RESOURCE, 0u },									   // Dynamic
			{ D3D11_USAGE_IMMUTABLE, 0u, D3D11_BIND_SHADER_RESOURCE, 0u },														   // Immutable
			{ D3D11_USAGE_DEFAULT, 0u, D3D11_BIND_RENDER_TARGET, 0u },															   // Colorbuffer
			{ D3D11_USAGE_DEFAULT, 0u, D3D11_BIND_DEPTH_STENCIL, 0u },															   // Depthbuffer
			{ D3D11_USAGE_DEFAULT, 0u, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_GENERATE_MIPS }, // Colortexture
			{ D3D11_USAGE_DEFAULT, 0u, D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE, 0u },								   // Depthtexture
		} };

		static ID3D11Resource* createTexture(ID3D11Device* device11, Texture::Type type, Texture::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, const TextureUsageD3D11& textureUsage) {
			ID3D11Resource* result = nullptr;
			HRESULT hr;

			DXGI_FORMAT format11 = (Texture::isFormatDepth(format) && (textureUsage.bindFlags & D3D11_BIND_SHADER_RESOURCE)) ? gTextureFormatDepthDescD3D11[format] : gTextureFormatD3D11[format];

			switch (type) {
			case Texture::Type_1D:
			case Texture::Type_1D_Array: {
				D3D11_TEXTURE1D_DESC desc = {};

				desc.Width = width;
				desc.MipLevels = mipLevels;
				desc.Format = format11;
				desc.ArraySize = (type == Texture::Type_1D_Array) ? depth : 1u;
				desc.Usage = textureUsage.usage;
				desc.BindFlags = textureUsage.bindFlags;
				desc.CPUAccessFlags = textureUsage.cpuAccess;
				desc.MiscFlags = textureUsage.misc;

				hr = device11->CreateTexture1D(&desc, nullptr, reinterpret_cast<ID3D11Texture1D**>(&result));

				break;
			}
			case Texture::Type_2D:
			case Texture::Type_2DMS:
			case Texture::Type_Cube: {
				D3D11_TEXTURE2D_DESC desc = {};
				uint32_t msaaCount = (type == Texture::Type_2DMS) ? 4u : 1u;
				uint32_t msaaQuality = 0u;

				if (type == Texture::Type_2DMS && FAILED(device11->CheckMultisampleQualityLevels(format11, msaaCount, &msaaQuality)))
					throw RBX::runtime_error("Unsupported multisampling level for 2D texture.");

				desc.Width = width;
				desc.Height = height;
				desc.MipLevels = mipLevels;
				desc.Format = format11;
				desc.SampleDesc.Count = msaaCount;
				desc.SampleDesc.Quality = msaaQuality ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0u;
				desc.ArraySize = (type == Texture::Type_Cube) ? 6u : 1u;
				desc.Usage = textureUsage.usage;
				desc.BindFlags = textureUsage.bindFlags;
				desc.CPUAccessFlags = textureUsage.cpuAccess;
				desc.MiscFlags = textureUsage.misc | ((type == Texture::Type_Cube) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0u);

				hr = device11->CreateTexture2D(&desc, nullptr, reinterpret_cast<ID3D11Texture2D**>(&result));

				break;
			}
			case Texture::Type_2D_Array:
			case Texture::Type_2DMS_Array:
			case Texture::Type_Cube_Array: {
				D3D11_TEXTURE2D_DESC desc = {};
				uint32_t msaaCount = (type == Texture::Type_2DMS_Array) ? 4u : 1u;
				uint32_t msaaQuality = 0u;

				if (type == Texture::Type_2DMS_Array && FAILED(device11->CheckMultisampleQualityLevels(format11, msaaCount, &msaaQuality)))
					throw RBX::runtime_error("Unsupported multisampling level for 2D texture array.");

				desc.Width = width;
				desc.Height = height;
				desc.MipLevels = mipLevels;
				desc.Format = format11;
				desc.SampleDesc.Count = msaaCount;
				desc.SampleDesc.Quality = msaaQuality ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0u;
				desc.ArraySize = ((type == Texture::Type_Cube_Array) ? 6u : 1u) * depth;
				desc.Usage = textureUsage.usage;
				desc.BindFlags = textureUsage.bindFlags;
				desc.CPUAccessFlags = textureUsage.cpuAccess;
				desc.MiscFlags = textureUsage.misc | ((type == Texture::Type_Cube_Array) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0u);

				hr = device11->CreateTexture2D(&desc, nullptr, reinterpret_cast<ID3D11Texture2D**>(&result));

				break;
			}
			case Texture::Type_3D: {
				D3D11_TEXTURE3D_DESC desc = {};

				desc.Width = width;
				desc.Height = height;
				desc.Depth = depth;
				desc.MipLevels = mipLevels;
				desc.Format = format11;
				desc.Usage = textureUsage.usage;
				desc.BindFlags = textureUsage.bindFlags;
				desc.CPUAccessFlags = textureUsage.cpuAccess;
				desc.MiscFlags = textureUsage.misc;

				hr = device11->CreateTexture3D(&desc, nullptr, reinterpret_cast<ID3D11Texture3D**>(&result));

				break;
			}
			default:
				RBXASSERT(false);
			}

			if (FAILED(hr))
				throw RBX::runtime_error("Error creating texture: %x", hr);

			return result;
		}

		static ID3D11ShaderResourceView* createSRV(ID3D11Device* device11, ID3D11Resource* resource, Texture::Type type, Texture::Format format, uint32_t mipLevels, uint32_t arraySize) {
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = Texture::isFormatDepth(format) ? gTextureFormatDepthViewD3D11[format] : gTextureFormatD3D11[format];

			switch (type) {
			case Texture::Type_1D: {
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
				srvDesc.Texture1D.MipLevels = mipLevels;
				srvDesc.Texture1D.MostDetailedMip = 0u;

				break;
			}
			case Texture::Type_1D_Array: {
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
				srvDesc.Texture1DArray.MipLevels = mipLevels;
				srvDesc.Texture1DArray.MostDetailedMip = 0u;
				srvDesc.Texture1DArray.FirstArraySlice = 0u;
				srvDesc.Texture1DArray.ArraySize = arraySize;

				break;
			}
			case Texture::Type_2D: {
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = mipLevels;
				srvDesc.Texture2D.MostDetailedMip = 0u;

				break;
			}
			case Texture::Type_2DMS: {
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;

				break;
			}
			case Texture::Type_2D_Array: {
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
				srvDesc.Texture2DArray.MipLevels = mipLevels;
				srvDesc.Texture2DArray.MostDetailedMip = 0u;
				srvDesc.Texture2DArray.FirstArraySlice = 0u;
				srvDesc.Texture2DArray.ArraySize = arraySize;

				break;
			}
			case Texture::Type_2DMS_Array: {
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
				srvDesc.Texture2DMSArray.FirstArraySlice = 0u;
				srvDesc.Texture2DMSArray.ArraySize = arraySize;

				break;
			}
			case Texture::Type_Cube: {
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				srvDesc.TextureCube.MipLevels = mipLevels;
				srvDesc.TextureCube.MostDetailedMip = 0u;

				break;
			}
			case Texture::Type_Cube_Array: {
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
				srvDesc.TextureCubeArray.MipLevels = mipLevels;
				srvDesc.TextureCubeArray.MostDetailedMip = 0u;
				srvDesc.TextureCubeArray.First2DArrayFace = 0u;
				srvDesc.TextureCubeArray.NumCubes = arraySize;

				break;
			}
			case Texture::Type_3D: {
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
				srvDesc.Texture3D.MipLevels = mipLevels;
				srvDesc.Texture3D.MostDetailedMip = 0u;

				break;
			}
			}

			ID3D11ShaderResourceView* SRV = nullptr;
			HRESULT hr = device11->CreateShaderResourceView(resource, &srvDesc, &SRV);

			if (FAILED(hr))
				throw RBX::runtime_error("Error creating shader resource view: %x", hr);

			return SRV;
		}

		TextureD3D11::TextureD3D11()
			: Texture()
			, object(nullptr)
			, objectSRV(nullptr)
		{
		}

		TextureD3D11::TextureD3D11(Device* device, Type type, Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, Usage usage)
			: Texture(device, type, format, width, height, depth, mipLevels, usage)
			, object(nullptr)
			, objectSRV(nullptr)
		{
			ID3D11Device* device11 = static_cast<DeviceD3D11*>(device)->getDevice11();

			object = createTexture(device11, type, format, width, height, depth, mipLevels, gTextureUsageD3D11[usage]);

			if (gTextureUsageD3D11[usage].bindFlags & D3D11_BIND_SHADER_RESOURCE)
				objectSRV = createSRV(device11, object, type, format, mipLevels, depth);
		}

		void TextureD3D11::upload(uint32_t index, uint32_t mip, const TextureRegion& region, const void* data, size_t size) {
			uint32_t mipWidth = getMipSide(width, mip);
			uint32_t mipHeight = getMipSide(height, mip);
			uint32_t mipDepth = getMipSide(depth, mip);

			RBXASSERT(mip < mipLevels);
			RBXASSERT(region.x + region.width <= mipWidth);
			RBXASSERT(region.y + region.height <= mipHeight);
			RBXASSERT(region.z + region.depth <= mipDepth);

			RBXASSERT(size == getImageSize(format, region.width, region.height) * region.depth);

			bool partialUpload = (region.width != mipWidth || region.height != mipHeight || region.depth != mipDepth);

			ID3D11DeviceContext* context11 = static_cast<DeviceD3D11*>(device)->getImmediateContext11();

			uint32_t dataRowPitch = Texture::getImageSize(format, region.width, 1u);
			uint32_t dataSlicePitch = (type == Type_3D) ? Texture::getImageSize(format, region.width, region.height) : 0u;

			D3D11_BOX box = {};
			if (partialUpload) {
				box.left = region.x;
				box.right = region.x + region.width;
				box.top = region.y;
				box.bottom = region.y + region.height;
				box.front = region.z;
				box.back = region.z + region.depth;
			}

			UINT res = D3D11CalcSubresource(mip, index, mipLevels);
			context11->UpdateSubresource(object, res, partialUpload ? &box : nullptr, data, dataRowPitch, dataSlicePitch);
		}

		static void copyHelper(void* targetData, uint32_t targetRowPitch, uint32_t targetSlicePitch,
			const void* sourceData, uint32_t sourceRowPitch, uint32_t sourceSlicePitch,
			uint32_t width, uint32_t height, uint32_t depth, Texture::Format format)
		{
			uint32_t heightBlocks = Texture::isFormatCompressed(format) ? (height + 3u) / 4u : height;
			uint32_t lineSize = Texture::getImageSize(format, width, 1u);

			RBXASSERT(lineSize <= sourceRowPitch && lineSize <= targetRowPitch);

			if (targetRowPitch == sourceRowPitch && targetSlicePitch == sourceSlicePitch) {
				// Fast path: memory layout is the same
				uint32_t size = (sourceSlicePitch == 0u) ? depth * heightBlocks * sourceRowPitch : depth * sourceSlicePitch;

				memcpy(targetData, sourceData, size);
			}
			else {
				// Slow path: need to copy line by line
				for (uint32_t z = 0u; z < depth; ++z)
					for (uint32_t yb = 0u; yb < heightBlocks; ++yb) {
						void* target = static_cast<char*>(targetData) + z * targetSlicePitch + yb * targetRowPitch;
						const void* source = static_cast<const char*>(sourceData) + z * sourceSlicePitch + yb * sourceRowPitch;

						memcpy(target, source, lineSize);
					}
			}
		}

		bool TextureD3D11::download(uint32_t index, uint32_t mip, void* data, size_t size) {
			uint32_t mipWidth = getMipSide(width, mip);
			uint32_t mipHeight = getMipSide(height, mip);
			uint32_t mipDepth = getMipSide(depth, mip);

			RBXASSERT(mip < mipLevels);
			//RBXASSERT(size == getImageSize(format, mipWidth, mipHeight) * mipDepth);
			if (type == Type::Type_3D)
				return false;

			DeviceD3D11* device11 = static_cast<DeviceD3D11*>(device);
			ID3D11DeviceContext* context11 = device11->getImmediateContext11();

			D3D11_TEXTURE2D_DESC desc = {};
			desc.Width = mipWidth;
			desc.Height = mipHeight;
			desc.MipLevels = 1u;
			desc.Format = gTextureFormatD3D11[format];
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
				throw RBX::runtime_error("Download texture cant create temp texture %x", hr);

			UINT res = D3D11CalcSubresource(mip, index, mipLevels);
			context11->CopySubresourceRegion(tempTex, 0u, 0u, 0u, 0u, object, res, nullptr);

			// copy texture to provided memory
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			hr = context11->Map(tempTex, 0u, D3D11_MAP_READ, 0u, &mappedResource);
			RBXASSERT(SUCCEEDED(hr));

			uint32_t dataRowPitch = Texture::getImageSize(format, mipWidth, 1u);
			uint32_t dataSlicePitch = (type == Type_3D) ? Texture::getImageSize(format, mipWidth, mipHeight) : 0u;

			copyHelper(data, dataRowPitch, dataSlicePitch, mappedResource.pData, mappedResource.RowPitch, 0u, mipWidth, mipHeight, mipDepth, format);

			// release all the things
			context11->Unmap(tempTex, 0u);
			ReleaseCheck(tempTex);

			return true;
		}

		bool TextureD3D11::supportsLocking() const {
			return false;
		}

		Texture::LockResult TextureD3D11::lock(uint32_t index, uint32_t mip, const TextureRegion& region) {
			return LockResult();
		}

		void TextureD3D11::unlock(uint32_t index, uint32_t mip)
		{
		}

		std::shared_ptr<Renderbuffer> TextureD3D11::getRenderbuffer(uint32_t index, uint32_t mip) {
			RBXASSERT(mip < mipLevels);

			std::weak_ptr<Renderbuffer>& slot = renderbuffers[std::make_pair(index, mip)];
			std::shared_ptr<Renderbuffer> result = slot.lock();

			if (!result) {
				Type type = getType();

				// TODO: Add Texture1D, Texture1DArray, and Texture3D support
				if (type == Type_1D || type == Type_1D_Array || type == Type_3D)
					throw std::runtime_error("Tried to create a renderbuffer for an unsupported texture type.");

				result.reset(new RenderbufferD3D11(device, shared_from_this(), index, mip, (type == Type_2DMS || type == Type_2DMS_Array) ? 4u : 1u));

				slot = result;
			}

			return result;
		}

		DXGI_FORMAT TextureD3D11::getInternalFormat(Texture::Format format) {
			return gTextureFormatD3D11[format];
		}

		void TextureD3D11::commitChanges()
		{
		}

		void TextureD3D11::generateMipmaps() {
			RBXASSERT(usage == Texture::Usage_Colorbuffer);

			ID3D11DeviceContext* context11 = static_cast<DeviceD3D11*>(device)->getImmediateContext11();

			context11->GenerateMips(objectSRV);
		}

		TextureD3D11::~TextureD3D11() {
			static_cast<DeviceD3D11*>(device)->getImmediateContextD3D11()->invalidateCachedTexture(this);

			ReleaseCheck(objectSRV);
			ReleaseCheck(object);
		}
	}
}
