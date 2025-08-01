#include "GfxCore/Framebuffer.h"

#include "rbx/Profiler.h"

namespace RBX {
	namespace Graphics {

		Renderbuffer::Renderbuffer(Device* device, Texture::Format format, uint32_t width, uint32_t height, uint32_t samples)
			: Resource(device)
			, format(format)
			, width(width)
			, height(height)
			, samples(samples)
		{
			RBXPROFILER_COUNTER_ADD("memory/gpu/renderbuffer", Texture::getImageSize(format, width, height) * samples);
		}

		Renderbuffer::~Renderbuffer() {
			RBXPROFILER_COUNTER_SUB("memory/gpu/renderbuffer", Texture::getImageSize(format, width, height) * samples);
		}

		Framebuffer::Framebuffer(Device* device, uint32_t width, uint32_t height, uint32_t samples)
			: Resource(device)
			, width(width)
			, height(height)
			, samples(samples)
		{
		}

		Framebuffer::~Framebuffer()
		{
		}

	}
}