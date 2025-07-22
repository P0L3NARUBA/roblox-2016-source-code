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
			
			VisualEngine*			visualEngine;
			TextureRef				outdoorTexture; // Outdoor cubemap texture
			TextureRef				indoorTextures; // Indoor cubemap textures
			shared_ptr<Framebuffer> outFaces[6]; // Outdoor cubemap framebuffers
			shared_ptr<Framebuffer> inFaces[24]; // Indoor cubemaps framebuffers
			//shared_ptr<ShaderProgram> mipGen; // mipmap generator
			unsigned                updateStep;
			DirtyState              dirtyState;
			double                  envmapLastTimeOfDay;
			double                  envmapLastRealTime;

			void renderOutdoorFace(DeviceContext* context, unsigned int face);
			virtual void onDeviceRestored();
			//void clearDirty();
		};

	}
} // ns

