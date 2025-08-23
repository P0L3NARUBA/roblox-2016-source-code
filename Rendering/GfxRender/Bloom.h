#pragma once

#include "ScreenSpaceEffect.h"

namespace RBX {
	namespace Graphics {

		class Bloom {
		public:
			struct Data {
				std::shared_ptr<Texture>     bloomTexture;
				std::shared_ptr<Framebuffer> bloomFB;

				std::vector<std::shared_ptr<Texture>>     bloomTextures;
				std::vector<std::shared_ptr<Framebuffer>> bloomFBs;

				uint32_t width;
				uint32_t height;
				float aspectRatio;
			};

			Bloom(VisualEngine* visualEngine);

			bool valid() { return data.get() != nullptr; }

			float getIntensity() const { return intensity; }
			const std::shared_ptr<Texture>& getTexture() const { return data->bloomTexture; }

			void update(uint32_t width, uint32_t height, float newIntensity, uint32_t newSize);
			void render(DeviceContext* context, const std::shared_ptr<Texture>& source, GlobalProcessingData globalProcessingData);

			Data* createData(uint32_t width, uint32_t height);

		private:
			bool bloomNeeded() const { return intensity > FLT_EPSILON && size != 0u; }

		private:
			VisualEngine* visualEngine;
			std::unique_ptr<Data> data;
			
			bool error;
			float intensity;
			uint32_t size;
		};
	}
}