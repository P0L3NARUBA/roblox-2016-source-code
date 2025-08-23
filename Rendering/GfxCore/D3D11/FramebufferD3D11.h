#pragma once

#include "GfxCore/Framebuffer.h"
#include "TextureD3D11.h"
#include <vector>

struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11View;
struct ID3D11Texture2D;
struct ID3D11Device;

namespace RBX {
	namespace Graphics {

		class RenderbufferD3D11 : public Renderbuffer {
		public:
			RenderbufferD3D11(Device* device, const std::shared_ptr<TextureD3D11>& owner, uint32_t cubeIndex, uint32_t mipIndex, uint32_t samples);
			RenderbufferD3D11(Device* device, Texture::Format format, uint32_t width, uint32_t height, uint32_t samples, ID3D11Texture2D* texture);
			RenderbufferD3D11(Device* device, Texture::Format format, uint32_t width, uint32_t height, uint32_t samples);
			~RenderbufferD3D11();

			ID3D11View* getObject() const { return object; }
			ID3D11Resource* getResource() const { return texture; }
			const std::shared_ptr<TextureD3D11>& getOwner() const { return owner; }

		private:
			ID3D11View* object;
			ID3D11Resource* texture;
			std::shared_ptr<TextureD3D11> owner;
		};

		class FramebufferD3D11 : public Framebuffer {
		public:
			FramebufferD3D11(Device* device, const std::vector<std::shared_ptr<Renderbuffer>>& color, const std::shared_ptr<Renderbuffer>& depth);
			~FramebufferD3D11();

			virtual void download(void* data, size_t size);

			const std::vector<std::shared_ptr<Renderbuffer>>& getColor() const { return color; }
			const std::shared_ptr<Renderbuffer>& getDepth() const { return depth; }

		private:
			std::vector<std::shared_ptr<Renderbuffer>> color;
			std::shared_ptr<Renderbuffer> depth;
		};

	}
}
