#pragma once

#include "ScreenSpaceEffect.h"

namespace RBX {
	namespace Graphics {

		class Bloom {
		public:
			struct Data {
				shared_ptr<Texture> bloomBuffer;
				std::vector<shared_ptr<Framebuffer>> bloomFBs;

				std::vector<uint32_t> width;
				std::vector<uint32_t> height;
			};

			Bloom(VisualEngine* visualEngine);

			bool valid() { return data.get() != nullptr; }

			float getIntensity() const { return intensity; }
			Texture* getTexture() const { return data->bloomBuffer.get(); }

			void update(uint32_t width, uint32_t height, float newIntensity, uint32_t newSize);
			void render(DeviceContext* context, Texture* source, GlobalProcessingData globalProcessingData);

			Data* createData(uint32_t width, uint32_t height);

		private:
			bool bloomNeeded() const { return intensity > FLT_EPSILON && size != 0u; }

		private:
			VisualEngine* visualEngine;
			scoped_ptr<Data> data;
			
			bool error;
			float intensity;
			uint32_t size;
		};
	}
}