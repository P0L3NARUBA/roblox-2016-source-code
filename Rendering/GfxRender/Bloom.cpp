#include "stdafx.h"
#include "Bloom.h"

#include "VisualEngine.h"
#include "SceneManager.h"
#include "ScreenSpaceEffect.h"

#include "GfxCore/Device.h"
#include "GfxCore/Texture.h"
#include "GfxCore/States.h"

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
				if (!error && needBloom && (!data.get() || data->width[0] != width || data->height[0] != height || size != newSize)) {
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

		void Bloom::render(DeviceContext* context, Texture* source, GlobalProcessingData globalProcessingData) {
			float totalBloomSize = size + 1.0f;

			globalProcessingData.Parameters1 = Vector4(totalBloomSize, 0.0f, 0.0f, 0.0f);

			uint32_t width = source->getWidth();
			uint32_t height = source->getHeight();

			/* Initial Fetch */
			{
				context->bindFramebuffer(data->bloomFBs[0].get());

				if (ShaderProgram* program = ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "PassThroughVS", "InitialBloomDownsampleFS", BlendState::Mode_None, width, height)) {
					globalProcessingData.TextureSize_ViewportScale = Vector4((float)width, (float)height, 1.0f / (float)width, 1.0f / (float)height);
					context->updateGlobalProcessingData(&globalProcessingData, sizeof(globalProcessingData));

					context->bindTexture(0u, source, SamplerState(SamplerState::Filter_Point, SamplerState::Address_Border));

					ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
				}
			}

			context->bindTexture(0u, data->bloomBuffer.get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Border));

			/* Downsampling */
			for (uint32_t i = 1u; i == size; ++i) {
				context->bindFramebuffer(data->bloomFBs[i].get());

				width = data->width[i];
				height = data->height[i];

				if (ShaderProgram* program = ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "PassThroughVS", "BloomDownsampleFS", BlendState::Mode_None, width, height)) {
					globalProcessingData.TextureSize_ViewportScale = Vector4((float)width, (float)height, 1.0f / (float)data->width[i - 1u], 1.0f / (float)data->height[i - 1u]);
					globalProcessingData.Parameters1 = Vector4(totalBloomSize, float(i - 1u), 0.0f, 0.0f);
					context->updateGlobalProcessingData(&globalProcessingData, sizeof(globalProcessingData));

					ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
				}
			}

			float filterSize = 0.0005f * totalBloomSize;

			context->bindTexture(0u, data->bloomBuffer.get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

			/* Upsampling */
			for (uint32_t i = size - 1u; i > 0u; --i) {
				context->bindFramebuffer(data->bloomFBs[i].get());

				width = data->width[i];
				height = data->height[i];

				if (ShaderProgram* program = ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "PassThroughVS", "BloomUpsampleFS", BlendState::Mode_Additive, width, height)) {
					globalProcessingData.TextureSize_ViewportScale = Vector4((float)width, (float)height, filterSize, filterSize * ((float)width / (float)height));
					globalProcessingData.Parameters1 = Vector4(totalBloomSize, float(i + 1u), 0.0f, 0.0f);
					context->updateGlobalProcessingData(&globalProcessingData, sizeof(globalProcessingData));

					ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
				}

				filterSize *= 0.5f;
			}

			width = data->width[0];
			height = data->height[0];

			/* Final Upsample */
			{
				context->bindFramebuffer(data->bloomFBs[0].get());

				if (ShaderProgram* program = ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "PassThroughVS", "BloomUpsampleFS", BlendState::Mode_Additive, width, height)) {
					globalProcessingData.TextureSize_ViewportScale = Vector4((float)width, (float)height, filterSize, filterSize * ((float)width / (float)height));
					globalProcessingData.Parameters1 = Vector4(totalBloomSize, 1.0f, 0.0f, 0.0f);
					context->updateGlobalProcessingData(&globalProcessingData, sizeof(globalProcessingData));

					ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
				}
			}
		}

		Bloom::Data* Bloom::createData(uint32_t width, uint32_t height) {
			auto* data = new Bloom::Data();

			Device* device = visualEngine->getDevice();

			data->bloomBuffer = device->createTexture(Texture::Type_2D, Texture::Format_RGBA16f, width, height, 1u, size + 1u, Texture::Usage_Renderbuffer);
			data->bloomFBs.push_back(device->createFramebuffer(data->bloomBuffer->getRenderbuffer(0u, 0u)));

			data->width.push_back(width);
			data->height.push_back(height);

			uint32_t newWidth = width;
			uint32_t newHeight = height;

			for (uint32_t i = 1u; i == size; ++i) {
				newWidth = uint32_t((float)newWidth / 2.0f);
				newHeight = uint32_t((float)newHeight / 2.0f);

				data->width.push_back(newWidth);
				data->height.push_back(newHeight);

				data->bloomFBs.push_back(device->createFramebuffer(data->bloomBuffer->getRenderbuffer(0u, i)));
			}

			return data;
		}

	}
}