

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

static const unsigned int outdoorCubemapSize = 1024u;
static const unsigned int indoorCubemapSize = 512u;
static const unsigned int irradianceCubemapSize = 32u;
static const unsigned int cubemapMips = 6u;

static const unsigned int indoorCubemapCount = 4u;
static const unsigned int indoorCubemapTextureCount = 6u * indoorCubemapCount;

namespace RBX {
	namespace Graphics {

		EnvMap::EnvMap(VisualEngine* ve) : Resource(ve->getDevice()) {
			dirtyState = VeryDirty;
			updateStep = 0;
			visualEngine = ve;
			Device* device = ve->getDevice();
			envmapLastRealTime = 0;
			envmapLastTimeOfDay = 0;
			setDebugName("Global envmap");

			outdoorTexture	   = device->createTexture(Texture::Type_Cube, Texture::Format_RGBA16f, outdoorCubemapSize, outdoorCubemapSize, 1u, cubemapMips, Texture::Usage_Renderbuffer);
			indoorTextures	   = device->createTexture(Texture::Type_Cube_Array, Texture::Format_RGBA16f, indoorCubemapSize, indoorCubemapSize, indoorCubemapCount, cubemapMips, Texture::Usage_Renderbuffer);
			irradianceTextures = device->createTexture(Texture::Type_Cube_Array, Texture::Format_RGBA16f, irradianceCubemapSize, irradianceCubemapSize, indoorCubemapCount + 1u, 1u, Texture::Usage_Renderbuffer);

			for (unsigned int i = 0u; i < 6u; ++i) {
				CubemapFace cubemapFace;

				for (unsigned int m = 0u; m < cubemapMips; ++m) {
					shared_ptr<Renderbuffer> rb = outdoorTexture.getTexture()->getRenderbuffer(i, m);

					cubemapFace.mips[m] = device->createFramebuffer(rb);
				}

				outFaces[i] = cubemapFace;
			}

			/*for (unsigned int i = 0u; i < indoorCubemapTextureCount; ++i) {
				shared_ptr<Renderbuffer> inrb = indoorTextures.getTexture()->getRenderbuffer(i, 0u);

				inFaces[i] = device->createFramebuffer(inrb);
			}*/

			/*for (unsigned int i = 0u; i < indoorCubemapTextureCount + 6u; ++i) {
				shared_ptr<Renderbuffer> irradiancerb = irradianceTextures.getTexture()->getRenderbuffer(i, 0u);

				irradianceFaces[i] = device->createFramebuffer(irradiancerb);
			}*/
		}

		EnvMap::~EnvMap()
		{
		}

		static Matrix4 lookat(Vector3 dir, Vector3 up)
		{
			Matrix4 result = Matrix4::identity();
			Vector3 rt = cross(up, -1.0f * dir);

			result.setRow(0, Vector4(rt, 0.0f));
			result.setRow(1, Vector4(up, 0.0f));
			result.setRow(2, Vector4(dir, 0.0f));

			return result;
		}


		static const double kEnvmapGameTimePeriod = 120;
		static const double kEnvmapRealtimePeriod = 30;


		void EnvMap::update(DeviceContext* context, double gameTime) {
			// prepare the envmap
			if (outdoorTexture.getTexture()) {
				double realTime = RBX::Time::nowFastSec();

				//if (visualEngine->getSettings()->getEagerBulkExecution() || fabs(envmapLastTimeOfDay - gameTime) > kEnvmapGameTimePeriod || fabs(envmapLastRealTime - realTime) > kEnvmapRealtimePeriod) {
					envmapLastTimeOfDay = gameTime;
					envmapLastRealTime = realTime;

					fullUpdate(context);
				//}
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
		void EnvMap::fullUpdate(DeviceContext* context) {
			Sky* sky = visualEngine->getSceneManager()->getSky();

			if (sky->getUseHDRI()) {
				static Matrix4 view[6u] = {
					lookat(Vector3(-1.0f,  0.0f,  0.0f), Vector3(0.0f,  1.0f,  0.0f)),
					lookat(Vector3( 1.0f,  0.0f,  0.0f), Vector3(0.0f,  1.0f,  0.0f)),
					lookat(Vector3( 0.0f, -1.0f,  0.0f), Vector3(0.0f,  0.0f, -1.0f)),
					lookat(Vector3( 0.0f,  1.0f,  0.0f), Vector3(0.0f,  0.0f,  1.0f)),
					lookat(Vector3( 0.0f,  0.0f, -1.0f), Vector3(0.0f,  1.0f,  0.0f)),
					lookat(Vector3( 0.0f,  0.0f,  1.0f), Vector3(0.0f,  1.0f,  0.0f)),
				};

				sky->PrepareSkyboxEnvMapEqui(context);

				SceneManager* sceneManager = visualEngine->getSceneManager();
				GlobalShaderData globalShaderData = sceneManager->readGlobalShaderData();
				RenderCamera cam;

				for (unsigned int i = 0u; i < 6u; ++i) {
					cam.setViewMatrix(view[i]);
					cam.setProjectionPerspective(MATH_PI / 2.0f, 1.0f, 0.1f, 10.0f);

					globalShaderData.setCamera(cam);

					context->updateGlobalConstants(&globalShaderData, sizeof(globalShaderData));
					context->bindFramebuffer(outFaces[i].mips[0u].get());
					
					sky->RenderSkyboxEnvMapEqui(context);
				}
			}
			else {
				for (unsigned int i = 0u; i < 6u; ++i) {
					context->bindFramebuffer(outFaces[i].mips[0u].get());

					sky->RenderSkyboxEnvMapCube(context, i, outdoorCubemapSize);
				}
			}

			// TODO: Add irradiance and convolution generation
		}

		void EnvMap::onDeviceRestored()
		{
		}


	}
}
