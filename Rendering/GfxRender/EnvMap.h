#pragma once

#include <rbx/Boost.hpp>
#include "GfxCore/Resource.h"
#include "TextureRef.h"
#include "SceneManager.h"
#include "Sky.h"

static const uint32_t cubemapMips = 6u;

static const uint32_t indoorCubemapCount = 4u;
static const uint32_t indoorCubemapTextureCount = 6u * indoorCubemapCount;

static const uint32_t outdoorCubemapSize = 1024u;
static const uint32_t indoorCubemapSize = 512u;
static const uint32_t irradianceCubemapSize = 32u;

namespace RBX {
	namespace Graphics {

		class Framebuffer;
		class VisualEngine;
		class DeviceContext;
		class RenderCamera;
		class DeviceContext;
		class Device;

		class EnvMap : public Resource {
		public:
			EnvMap(VisualEngine* ve);
			~EnvMap();

			const std::shared_ptr<Texture>& getOutdoorTexture() const { return intermediateOutdoorTexture; }
			const std::shared_ptr<Texture>& getIndoorTextures() const { return indoorTextures; }
			const std::shared_ptr<Texture>& getIrradianceTextures() const { return irradianceTextures; }

			void update(DeviceContext* context, double gameTime);

			void setUpdateRequired(bool value) {
				updateRequired = value;
			}

		private:
			struct CubemapFace {
				std::array<std::shared_ptr<Framebuffer>, cubemapMips> mips;
			};
			
			VisualEngine* visualEngine;

			std::shared_ptr<Texture> outdoorTexture;     // Outdoor cubemap texture
			std::shared_ptr<Texture> indoorTextures;     // Indoor cubemap textures
			std::shared_ptr<Texture> irradianceTextures; // Irradiance cubemap textures

			std::shared_ptr<Texture> intermediateOutdoorTexture; // Intermediate outdoor cubemap texture
			std::shared_ptr<Texture> intermediateIndoorTexture;  // Intermediate indoor cubemap texture

			std::array<CubemapFace, 6u>												 outFaces;        // Outdoor cubemap framebuffers
			std::vector<CubemapFace>												 inFaces;         // Indoor cubemaps framebuffers
			std::array<std::shared_ptr<Framebuffer>, indoorCubemapTextureCount + 6u> irradianceFaces; // Irradiance cubemap framebuffers

			std::array<std::shared_ptr<Framebuffer>, 11u> intermediateOutFaces; // Intermediate outdoor cubemap framebuffers
			std::array<std::shared_ptr<Framebuffer>, 10u> intermediateInFaces;  // Intermediate indoor cubemap framebuffers

			bool   updateRequired;
			double envmapLastTimeOfDay;
			double envmapLastRealTime;

			void setFilter(DeviceContext* context, std::string type);

			void faceIrradiance(DeviceContext* context, Sky* sky, uint32_t face);
			void faceConvolution(DeviceContext* context, Sky* sky, CubemapFace cubemapFace, GlobalShaderData globalShaderData);

			//void incrementalUpdate(DeviceContext* context);
			void fullUpdate(DeviceContext* context);

			virtual void onDeviceRestored();
			//void clearDirty();
		};

	}
} // ns

