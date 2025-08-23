#include "stdafx.h"
#include "Bloom.h"

#include "VisualEngine.h"
#include "SceneManager.h"
#include "ScreenSpaceEffect.h"

#include "GfxCore/Device.h"
#include "GfxCore/Texture.h"
#include "GfxCore/States.h"
#include "GfxCore/Framebuffer.h"

namespace RBX {
	namespace Graphics {

		Bloom::Bloom(VisualEngine* visualEngine)
			: visualEngine(visualEngine)
			, error(false)
			, intensity(0.0f)
			, size(0u)
		{
		}

		void Bloom::update(uint32_t width, uint32_t height, float newIntensity, uint32_t newSize) {
			bool needBloom = bloomNeeded();

			if (data && !needBloom)
				data.reset();
			else {
				if (!error && needBloom && (!data.get() || data->width != width || data->height != height || size != newSize)) {
					intensity = newIntensity;
					size = newSize;

					try {
						data.reset(createData(width, height));
					}
					catch (const RBX::base_exception&) {
						error = true;

						data.reset();

						throw std::runtime_error("Failed to create data required for bloom.");
					}
				}
			}
		}

		void Bloom::render(DeviceContext* context, const std::shared_ptr<Texture>& source, GlobalProcessingData globalProcessingData) {
			float totalBloomSize = size + 1.0f;
			float filterSize = 0.0005f * totalBloomSize;
			float aspectRatio = data->aspectRatio;

			globalProcessingData.Parameters1 = Vector4(totalBloomSize, 0.0f, 0.0f, 0.0f);

			uint32_t iWidth = source->getWidth();
			uint32_t iHeight = source->getHeight();
			float fWidth = static_cast<float>(iWidth);
			float fHeight = static_cast<float>(iHeight);

			/* Initial Fetch */
			{
				context->bindFramebuffer(data->bloomFB.get());

				if (ShaderProgram* program = ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "PassThroughVS", "InitialBloomDownsampleFS", BlendState::Mode_None, iWidth, iHeight)) {
					globalProcessingData.TextureSize_ViewportScale = Vector4(fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight);
					context->updateGlobalProcessingData(&globalProcessingData, sizeof(globalProcessingData));

					context->bindTexture(0u, source);

					ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
				}
			}

			/* Downsampling */
			for (uint32_t i = 0u; i < size; ++i) {
				context->bindFramebuffer(data->bloomFBs[i].get());

				std::shared_ptr<Texture> texture = (i == 0u) ? data->bloomTexture : data->bloomTextures[i - 1u];
				iWidth = texture->getWidth();
				iHeight = texture->getHeight();
				fWidth = static_cast<float>(iWidth);
				fHeight = static_cast<float>(iHeight);

				if (ShaderProgram* program = ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "PassThroughVS", "BloomDownsampleFS", BlendState::Mode_None, iWidth, iHeight)) {
					globalProcessingData.TextureSize_ViewportScale = Vector4(fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight);
					globalProcessingData.Parameters1 = Vector4(totalBloomSize, 0.0f, 0.0f, 0.0f);
					context->updateGlobalProcessingData(&globalProcessingData, sizeof(globalProcessingData));

					context->bindTexture(0u, texture);

					ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
				}
			}

			/* Upsampling */
			for (uint32_t i = size - 1u; i > 0u; --i) {
				Framebuffer* framebuffer = data->bloomFBs[i - 1u].get();

				context->bindFramebuffer(framebuffer);

				iWidth = framebuffer->getWidth();
				iHeight = framebuffer->getHeight();
				fWidth = static_cast<float>(iWidth);
				fHeight = static_cast<float>(iHeight);

				if (ShaderProgram* program = ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "PassThroughVS", "BloomUpsampleFS", BlendState::Mode_Additive, iWidth, iHeight)) {
					globalProcessingData.TextureSize_ViewportScale = Vector4(fWidth, fHeight, filterSize, filterSize * aspectRatio);
					globalProcessingData.Parameters1 = Vector4(totalBloomSize, 0.0f, 0.0f, 0.0f);
					context->updateGlobalProcessingData(&globalProcessingData, sizeof(globalProcessingData));

					context->bindTexture(0u, data->bloomTextures[i]);

					ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
				}

				filterSize *= 0.5f;
			}

			iWidth = data->width;
			iHeight = data->height;
			fWidth = static_cast<float>(iWidth);
			fHeight = static_cast<float>(iHeight);

			/* Final Upsample */
			{
				context->bindFramebuffer(data->bloomFB.get());

				if (ShaderProgram* program = ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "PassThroughVS", "BloomUpsampleFS", BlendState::Mode_Additive, iWidth, iHeight)) {
					globalProcessingData.TextureSize_ViewportScale = Vector4(fWidth, fHeight, filterSize, filterSize * aspectRatio);
					globalProcessingData.Parameters1 = Vector4(totalBloomSize, 0.0f, 0.0f, 0.0f);
					context->updateGlobalProcessingData(&globalProcessingData, sizeof(globalProcessingData));

					context->bindTexture(0u, data->bloomTextures[0]);

					ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
				}
			}
		}

		Bloom::Data* Bloom::createData(uint32_t width, uint32_t height) {
			auto* data = new Bloom::Data();

			Device* device = visualEngine->getDevice();

			data->bloomTexture = device->createTexture(Texture::Type_2D, Texture::Format_R11G11B10f, width, height, 1u, 1u, Texture::Usage_Colortexture);
			data->bloomFB = device->createFramebuffer(data->bloomTexture->getRenderbuffer(0u, 0u));

			data->width = width;
			data->height = height;
			data->aspectRatio = static_cast<float>(width) / static_cast<float>(height);

			uint32_t newWidth = width;
			uint32_t newHeight = height;

			data->bloomTextures.reserve(size);

			for (uint32_t i = 0u; i < size; ++i) {
				newWidth = newWidth / 2u;
				newHeight = newHeight / 2u;
				
				data->bloomTextures.push_back(device->createTexture(Texture::Type_2D, Texture::Format_R11G11B10f, newWidth, newHeight, 1u, 1u, Texture::Usage_Colortexture));
				data->bloomFBs.push_back(device->createFramebuffer(data->bloomTextures[i]->getRenderbuffer(0u, 0u)));
			}

			return data;
		}

	}
}