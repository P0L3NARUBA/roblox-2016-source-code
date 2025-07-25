#pragma once

#include <rbx/Boost.hpp>
#include "GfxCore/Resource.h"
#include "TextureRef.h"

namespace RBX {
	namespace Graphics {

		class Framebuffer;
		class VisualEngine;
		class DeviceContext;
		class RenderCamera;
		class DeviceContext;
		class Device;

		class EnvMap : public Resource
		{
		public:
			EnvMap(VisualEngine* ve);
			~EnvMap();

			const TextureRef& getOutdoorTexture() const { return outdoorTexture; }
			const TextureRef& getIndoorTextures() const { return indoorTextures; }
			const TextureRef& getIrradianceTextures() const { return irradianceTextures; }

			void update(DeviceContext* context, double gameTime);
			//void markDirty(bool veryDirty = false);

		private:

			//void incrementalUpdate(DeviceContext* context);
			void fullUpdate(DeviceContext* context);
		private:

			enum DirtyState
			{
				Ready,       // ready for business
				Dirty,       // update has been scheduled
				BusyDirty,   // update has been scheduled but another update is already in progress
				DirtyWait,   // full update has been scheduled, but waiting for the sky to finish loading 
				VeryDirty,   // full update has been scheduled,
			};

			struct CubemapFace {
				shared_ptr<Framebuffer> mips[6];
			};
			
			VisualEngine*			visualEngine;
			TextureRef				outdoorTexture; // Outdoor cubemap texture
			TextureRef				indoorTextures; // Indoor cubemap textures
			TextureRef				irradianceTextures; // Irradiance cubemap textures
			CubemapFace				outFaces[6]; // Outdoor cubemap framebuffers
			CubemapFace				inFaces[24]; // Indoor cubemaps framebuffers
			shared_ptr<Framebuffer> irradianceFaces[30]; // Irradiance cubemap framebuffers
			//shared_ptr<ShaderProgram> mipGen; // mipmap generator
			unsigned                updateStep;
			DirtyState              dirtyState;
			double                  envmapLastTimeOfDay;
			double                  envmapLastRealTime;

			virtual void onDeviceRestored();
			//void clearDirty();
		};

	}
} // ns

