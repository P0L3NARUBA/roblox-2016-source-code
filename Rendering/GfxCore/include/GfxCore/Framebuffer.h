#pragma once

#include "Resource.h"
#include "Texture.h"

namespace RBX {
	namespace Graphics {

		class Renderbuffer : public Resource {
		public:
			~Renderbuffer();

			Texture::Format getFormat() const { return format; }

			uint32_t getWidth() const { return width; }
			uint32_t getHeight() const { return height; }
			uint32_t getSamples() const { return samples; }

		protected:
			Renderbuffer(Device* device, Texture::Format format, uint32_t width, uint32_t height, uint32_t samples);

			Texture::Format format;
			uint32_t width;
			uint32_t height;
			uint32_t samples;
		};

		class Framebuffer : public Resource {
		public:
			~Framebuffer();

			virtual void download(void* data, uint32_t size) = 0;

			uint32_t getWidth() const { return width; }
			uint32_t getHeight() const { return height; }
			uint32_t getSamples() const { return samples; }

		protected:
			Framebuffer(Device* device, uint32_t width, uint32_t height, uint32_t samples);

			uint32_t width;
			uint32_t height;
			uint32_t samples;
		};

	}
}
