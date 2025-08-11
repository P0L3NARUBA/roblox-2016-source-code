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

			const TextureRef& getOutdoorTexture() const { return outdoorTexture; }
			const TextureRef& getIndoorTextures() const { return indoorTextures; }
			const TextureRef& getIrradianceTextures() const { return irradianceTextures; }

			void update(DeviceContext* context, double gameTime);
			//void markDirty(bool veryDirty = false);

			void setUpdateRequired(bool value) {
				updateRequired = value;
			}

		private:

			/*enum DirtyState {
				Ready,       // ready for business
				Dirty,       // update has been scheduled
				BusyDirty,   // update has been scheduled but another update is already in progress
				DirtyWait,   // full update has been scheduled, but waiting for the sky to finish loading 
				VeryDirty,   // full update has been scheduled,
			};*/

			struct CubemapFace {
				shared_ptr<Framebuffer> mips[cubemapMips];
			};
			
			VisualEngine* visualEngine;

			TextureRef outdoorTexture; // Outdoor cubemap texture
			TextureRef indoorTextures; // Indoor cubemap textures
			TextureRef irradianceTextures; // Irradiance cubemap textures

			TextureRef intermediateOutdoorTexture; // Intermediate outdoor cubemap texture
			TextureRef intermediateIndoorTexture; // Intermediate indoor cubemap texture

			CubemapFace				 outFaces[6]; // Outdoor cubemap framebuffers
			std::vector<CubemapFace> inFaces; // Indoor cubemaps framebuffers
			shared_ptr<Framebuffer>  irradianceFaces[indoorCubemapTextureCount + 6u]; // Irradiance cubemap framebuffers

			shared_ptr<Framebuffer> intermediateOutFaces[11]; // Intermediate outdoor cubemap framebuffers
			shared_ptr<Framebuffer> intermediateInFaces[10]; // Intermediate indoor cubemap framebuffers

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

