

#include "stdafx.h"
#include "EnvMap.h"

#include "VisualEngine.h"
#include "GfxCore/Device.h"
#include "GfxCore/Texture.h"
#include "GfxCore/FrameBuffer.h"
#include "GlobalShaderData.h"
#include "SceneManager.h"
#include "ShaderManager.h"
#include "Sky.h"
#include "TextureManager.h"

#include "GfxCore/pix.h"

static const float MATH_PI = 3.14159265359f;

static const int outdoorCubemapSize = 1024;
static const int indoorCubemapSize = 512;

namespace RBX {
	namespace Graphics {

		EnvMap::EnvMap(VisualEngine* ve) : Resource(ve->getDevice())
		{
			dirtyState = VeryDirty;
			updateStep = 0;
			visualEngine = ve;
			Device* device = ve->getDevice();
			envmapLastRealTime = 0;
			envmapLastTimeOfDay = 0;
			setDebugName("Global envmap");

			outdoorTexture = device->createTexture(Texture::Type_Cube, Texture::Format_BC6UF, outdoorCubemapSize, outdoorCubemapSize, 1, 6, Texture::Usage_Renderbuffer);
			indoorATexture = device->createTexture(Texture::Type_Cube, Texture::Format_RGBA16F, indoorCubemapSize, indoorCubemapSize, 1, 6, Texture::Usage_Renderbuffer);
			indoorBTexture = device->createTexture(Texture::Type_Cube, Texture::Format_RGBA16F, indoorCubemapSize, indoorCubemapSize, 1, 6, Texture::Usage_Renderbuffer);

			for (int i = 0; i < 6; ++i) {
				shared_ptr<Renderbuffer> outrb = outdoorTexture.getTexture()->getRenderbuffer(i, 0);
				shared_ptr<Renderbuffer> inArb = indoorATexture.getTexture()->getRenderbuffer(i, 0);
				shared_ptr<Renderbuffer> inBrb = indoorBTexture.getTexture()->getRenderbuffer(i, 0);

				outFaces[i] = device->createFramebuffer(outrb);
				inAFaces[i] = device->createFramebuffer(inArb);
				inBFaces[i] = device->createFramebuffer(inBrb);
			}
		}

		EnvMap::~EnvMap()
		{

		}

		static Matrix4 lookat(Vector3 dir, Vector3 up, float rh = 1.0f)
		{
			Matrix4 result = Matrix4::identity();
			Vector3 rt = cross(up, rh * dir);
			result.setRow(0, Vector4(rt, 0));
			result.setRow(1, Vector4(up, 0));
			result.setRow(2, Vector4(dir, 0));
			return result;
		}


		static const double kEnvmapGameTimePeriod = 120;
		static const double kEnvmapRealtimePeriod = 30;


		void EnvMap::update(DeviceContext* context, double gameTime)
		{
			// prepare the envmap
			if (outdoorTexture.getTexture())
			{
				double realTime = RBX::Time::nowFastSec();

				if (fabs(envmapLastTimeOfDay - gameTime) > kEnvmapGameTimePeriod || fabs(envmapLastRealTime - realTime) > kEnvmapRealtimePeriod || visualEngine->getSettings()->getEagerBulkExecution())
				{
					envmapLastTimeOfDay = gameTime;
					envmapLastRealTime = realTime;

					fullUpdate(context);
				}
			}
		}

		/*void EnvMap::markDirty(bool veryDirty)
		{
			if( veryDirty )
			{
				dirtyState = DirtyWait;
			}
			else switch (dirtyState)
			{
			case VeryDirty:  case DirtyWait: break;
			case Dirty: dirtyState = BusyDirty; break;
			case BusyDirty: break;
			default:    dirtyState = Dirty; break;
			}
		}

		void EnvMap::clearDirty()
		{
			switch (dirtyState)
			{
			case Dirty:
				dirtyState = Ready;
				break;
			case BusyDirty:
				dirtyState = Dirty;
				break;
			case VeryDirty:
			case DirtyWait:
				if (visualEngine->getSceneManager()->getSky()->isReady())
					dirtyState = Ready;
				else
					dirtyState = DirtyWait;
				break;
			default:
				break;
			}
		}

		void EnvMap::incrementalUpdate(DeviceContext* context)
		{
			if (!texture.getTexture())
				return;

			if (dirtyState == Ready)
				return;

			if (updateStep>=7)
			{
				updateStep = 0;
				clearDirty();
				return;
			}

			if (updateStep<6)
			   renderFace(context,updateStep);
			else if (updateStep == 6)
			   texture.getTexture()->generateMipmaps();

			updateStep++;
		}*/

		// full re-render in a single frame
		void EnvMap::fullUpdate(DeviceContext* context)
		{
			/*
			while( !visualEngine->getSceneManager()->getSky()->isReady() )
			{
				visualEngine->getSceneManager()->getSky()->setSkyBoxDefault();
				visualEngine->getSceneManager()->getSky()->prerender(context);
				Sleep(100);
			}
			*/

			if (!outdoorTexture.getTexture())
				return;

			for (int i = 0; i < 6; ++i)
				renderFace(context, i);

			//texture.getTexture()->generateMipmaps();
		}

		void EnvMap::renderFace(DeviceContext* context, int face)
		{
			PIX_SCOPE(context, "EnvMap/update", 0);

			SceneManager* sman = visualEngine->getSceneManager();

			// THIS IS A HACK!
			/*static Matrix4 view[6 * 2] = {
				lookat(Vector3(-1,  0,  0), Vector3( 0,  1,  0), -1),
				lookat(Vector3( 1,  0,  0), Vector3( 0,  1,  0), -1),
				lookat(Vector3( 0, -1,  0), Vector3( 0,  0, -1), -1),
				lookat(Vector3( 0,  1,  0), Vector3( 0,  0,  1), -1),
				lookat(Vector3( 0,  0, -1), Vector3( 0,  1,  0), -1),
				lookat(Vector3( 0,  0,  1), Vector3( 0,  1,  0), -1),

				lookat(Vector3(-1,  0,  0), Vector3( 0, -1,  0)),
				lookat(Vector3( 1,  0,  0), Vector3( 0, -1,  0)),
				lookat(Vector3( 0, -1,  0), Vector3( 0,  0,  1)),
				lookat(Vector3( 0,  1,  0), Vector3( 0,  0, -1)),
				lookat(Vector3( 0,  0, -1), Vector3( 0, -1,  0)),
				lookat(Vector3( 0,  0,  1), Vector3( 0, -1,  0)),
			};*/

			{
				PIX_SCOPE(context, "Outdoor Environment Map face %d", face);

				context->bindFramebuffer(outFaces[face].get());

				visualEngine->getSceneManager()->getSky()->RenderSkyboxEnvMap(context, face);
				// add more stuff to render to envmap 
			}
		}

		void EnvMap::onDeviceRestored()
		{
			//markDirty(true);
		}


	}
}
