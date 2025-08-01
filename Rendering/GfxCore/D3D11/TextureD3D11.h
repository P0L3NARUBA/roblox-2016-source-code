#pragma once

#include "GfxCore/Texture.h"
#include "GfxCore/States.h"
#include <boost/enable_shared_from_this.hpp>
#include <map>

struct ID3D11Resource;
struct ID3D11ShaderResourceView;
enum DXGI_FORMAT;

namespace RBX {
	namespace Graphics {
		class Renderbuffer;

		class TextureD3D11 : public Texture, public boost::enable_shared_from_this<TextureD3D11> {
		public:
			TextureD3D11(Device* device, Type type, Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, Usage usage);
			~TextureD3D11();

			virtual void upload(uint32_t index, uint32_t mip, const TextureRegion& region, const void* data,  size_t size);
			virtual bool download(uint32_t index, uint32_t mip, void* data, size_t size);

			virtual bool supportsLocking() const;
			virtual LockResult lock(uint32_t index, uint32_t mip, const TextureRegion& region);
			virtual void unlock(uint32_t index, uint32_t mip);

			virtual shared_ptr<Renderbuffer> getRenderbuffer(uint32_t index, uint32_t mip);

			virtual void commitChanges();
			virtual void generateMipmaps();

			ID3D11ShaderResourceView* getSRV() const { return objectSRV; }
			ID3D11Resource* getObject() const { return object; }

			static uint32_t getInternalFormat(Texture::Format format);

		private:
			ID3D11ShaderResourceView* objectSRV;
			ID3D11Resource* object;

			typedef std::map<std::pair<uint32_t, uint32_t>, weak_ptr<Renderbuffer> > RenderbufferMap;
			RenderbufferMap renderbuffers;
		};

	}
}
