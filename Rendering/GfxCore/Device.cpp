#include "GfxCore/Device.h"

LOGVARIABLE(Graphics, 0)
LOGVARIABLE(VR, 0)
FASTFLAGVARIABLE(DebugGraphicsCrashOnLeaks, true)

FASTFLAGVARIABLE(GraphicsDebugMarkersEnable, false)

namespace RBX {
	namespace Graphics {
		DeviceContext::DeviceContext()
		{
		}

		DeviceContext::~DeviceContext()
		{
		}

		void DeviceContext::draw(Geometry* geometry, Geometry::Primitive primitive, uint32_t vertexCount, uint32_t vertexOffset, uint32_t indexOffset) {
			drawImpl(geometry, primitive, vertexCount, vertexOffset, indexOffset);
		}

		void DeviceContext::draw(const GeometryBatch& geometryBatch) {
			drawImpl(geometryBatch.getGeometry(), geometryBatch.getPrimitive(), geometryBatch.getVertexCount(), geometryBatch.getVerexOffset(), geometryBatch.getIndexOffset());
		}

		void DeviceContext::drawInstanced(Geometry* geometry, Geometry::Primitive primitive, uint32_t instanceCount, uint32_t vertexCount, uint32_t vertexOffset, uint32_t indexOffset) {
			drawInstancedImpl(geometry, primitive, instanceCount, vertexCount, vertexOffset, indexOffset);
		}

		void DeviceContext::drawInstanced(const GeometryBatch& geometryBatch, uint32_t instanceCount) {
			drawInstancedImpl(geometryBatch.getGeometry(), geometryBatch.getPrimitive(), instanceCount, geometryBatch.getVertexCount(), geometryBatch.getVerexOffset(), geometryBatch.getIndexOffset());
		}

		void DeviceCaps::dumpToFLog(int channel) const {
			FASTLOG2(channel, "Caps: Shaders %d FFP %d", supportsShaders, supportsFFP);
			FASTLOG5(channel, "Caps: Framebuffer %d (%d MRT, %d MSAA) Stencil %d 32bIdx %d", supportsFramebuffer, maxDrawBuffers, maxSamples, supportsStencil, supportsIndex32);
			FASTLOG4(channel, "Caps: Texture: DXT %d PVR %d ETC1 %d Half %d", supportsTextureDXT, supportsTexturePVR, supportsTextureETC1, supportsTextureHalfFloat);
			FASTLOG3(channel, "Caps: Texture: 3D %d NPOT %d PartialMips %d", supportsTexture3D, supportsTextureNPOT, supportsTexturePartialMipChain);
			//FASTLOG2(channel, "Caps: Texture: Size %d Units %d", maxTextureSize, maxTextureUnits);
			FASTLOG3(channel, "Caps: ColorBGR %d HalfPixelOffset %d RTFlip %d", colorOrderBGR, needsHalfPixelOffset, requiresRenderTargetFlipping);
			FASTLOG1(channel, "Caps: Retina %d", retina);
		}

		DeviceVR::~DeviceVR()
		{
		}

		Device::Device()
			: resourceListHead(nullptr)
			, resourceListTail(nullptr)
		{
		}

		Device::~Device() {
			if (resourceListHead) {
				FASTLOG(FLog::Graphics, "ERROR: Not all resources are destroyed!");

				unsigned int index = 0u;

				for (Resource* cur = resourceListHead; cur; cur = cur->next, index++) {
					FASTLOG2(FLog::Graphics, "Leak %d: resource %p", index, cur);
					FASTLOGS(FLog::Graphics, "Resource type %s", typeid(*cur).name());

					if (!cur->getDebugName().empty())
						FASTLOGS(FLog::Graphics, "Resource name %s", cur->getDebugName());
				}

				if (FFlag::DebugGraphicsCrashOnLeaks)
					RBXCRASH();
				else
					RBXASSERT(false);
			}
		}

		std::shared_ptr<Geometry> Device::createGeometry(const std::shared_ptr<VertexLayout>& layout, const std::shared_ptr<VertexBuffer>& vertexBuffer, const std::shared_ptr<IndexBuffer>& indexBuffer, uint32_t baseVertexIndex) {
			return createGeometryImpl(layout, vertexBuffer, indexBuffer, baseVertexIndex);
		}

		std::shared_ptr<Framebuffer> Device::createFramebuffer(const std::shared_ptr<Renderbuffer>& color, const std::shared_ptr<Renderbuffer>& depth) {
			std::vector<std::shared_ptr<Renderbuffer>> colors;
			colors.push_back(color);

			return createFramebufferImpl(colors, depth);
		}

		std::shared_ptr<Framebuffer> Device::createFramebuffer(const std::vector<std::shared_ptr<Renderbuffer>>& color, const std::shared_ptr<Renderbuffer>& depth) {
			return createFramebufferImpl(color, depth);
		}

		void Device::fireDeviceLost() {
			for (Resource* cur = resourceListTail; cur; cur = cur->prev)
				cur->onDeviceLost();
		}

		void Device::fireDeviceRestored() {
			for (Resource* cur = resourceListHead; cur; cur = cur->next)
				cur->onDeviceRestored();
		}

	}
}
