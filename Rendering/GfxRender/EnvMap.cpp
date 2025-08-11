

#include "stdafx.h"
#include "EnvMap.h"

#include "VisualEngine.h"
#include "GlobalShaderData.h"
#include "ShaderManager.h"
#include "TextureManager.h"

#include "GfxCore/Device.h"
#include "GfxCore/Texture.h"
#include "GfxCore/FrameBuffer.h"
#include "GfxCore/States.h"

#include "GfxCore/pix.h"

static const float MATH_PI = 3.14159265359f;

namespace RBX {
	namespace Graphics {

		EnvMap::EnvMap(VisualEngine* ve) : Resource(ve->getDevice()) {
			visualEngine = ve;
			envmapLastRealTime = 0.0;
			envmapLastTimeOfDay = 0.0;
			setDebugName("Global envmap");

			Device* device = ve->getDevice();

			outdoorTexture = device->createTexture(Texture::Type_Cube, Texture::Format_R11G11B10f, outdoorCubemapSize, outdoorCubemapSize, 1u, cubemapMips, Texture::Usage_Renderbuffer);
			indoorTextures = device->createTexture(Texture::Type_Cube_Array, Texture::Format_RGBA16f, indoorCubemapSize, indoorCubemapSize, indoorCubemapCount, cubemapMips, Texture::Usage_Renderbuffer);
			irradianceTextures = device->createTexture(Texture::Type_Cube_Array, Texture::Format_RGBA16f, irradianceCubemapSize, irradianceCubemapSize, indoorCubemapCount + 1u, 1u, Texture::Usage_Renderbuffer);

			intermediateOutdoorTexture = device->createTexture(Texture::Type_Cube, Texture::Format_R11G11B10f, outdoorCubemapSize, outdoorCubemapSize, 1u, 11u, Texture::Usage_Renderbuffer);
			intermediateIndoorTexture = device->createTexture(Texture::Type_Cube, Texture::Format_RGBA16f, indoorCubemapSize, indoorCubemapSize, 1u, 10u, Texture::Usage_Renderbuffer);

			for (uint32_t i = 0u; i < 6u; ++i) {
				CubemapFace cubemapFace;

				for (uint32_t m = 0u; m < cubemapMips; ++m) {
					shared_ptr<Renderbuffer> rb = outdoorTexture.getTexture()->getRenderbuffer(i, m);

					cubemapFace.mips[m] = device->createFramebuffer(rb);
				}

				outFaces[i] = cubemapFace;
			}

			for (uint32_t i = 0u; i < 6u; ++i) {
				CubemapFace cubemapFace;

				for (uint32_t m = 0u; m < cubemapMips; ++m) {
					shared_ptr<Renderbuffer> rb = indoorTextures.getTexture()->getRenderbuffer(i, m);

					cubemapFace.mips[m] = device->createFramebuffer(rb);
				}

				inFaces.push_back(cubemapFace);
			}

			for (uint32_t i = 0u; i < indoorCubemapTextureCount + 6u; ++i) {
				shared_ptr<Renderbuffer> irradiancerb = irradianceTextures.getTexture()->getRenderbuffer(i, 0u);

				irradianceFaces[i] = device->createFramebuffer(irradiancerb);
			}

			for (uint32_t i = 0u; i < 6u; ++i) {
				shared_ptr<Renderbuffer> rb = intermediateOutdoorTexture.getTexture()->getRenderbuffer(i, 0u);

				intermediateOutFaces[i] = device->createFramebuffer(rb);
			}

			for (uint32_t i = 0u; i < 6u; ++i) {
				shared_ptr<Renderbuffer> rb = intermediateIndoorTexture.getTexture()->getRenderbuffer(i, 0u);

				intermediateInFaces[i] = device->createFramebuffer(rb);
			}
		}

		EnvMap::~EnvMap()
		{
		}

		static const double kEnvmapGameTimePeriod = 120.0;
		static const double kEnvmapRealtimePeriod = 30.0;


		void EnvMap::update(DeviceContext* context, double gameTime) {
			if ((updateRequired || visualEngine->getSettings()->getEagerBulkExecution()) && outdoorTexture.getTexture() && intermediateOutdoorTexture.getTexture()) {
				double realTime = RBX::Time::nowFastSec();

				//if (visualEngine->getSettings()->getEagerBulkExecution() || fabs(envmapLastTimeOfDay - gameTime) > kEnvmapGameTimePeriod || fabs(envmapLastRealTime - realTime) > kEnvmapRealtimePeriod) {
				envmapLastTimeOfDay = gameTime;
				envmapLastRealTime = realTime;

				fullUpdate(context);

				updateRequired = false;
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

		static Matrix4 lookAt(Vector3 direction, Vector3 up) {
			Matrix4 result = Matrix4::identity();
			Vector3 right = cross(up, -1.0f * direction);

			result.setRow(0u, Vector4(right, 0.0f));
			result.setRow(1u, Vector4(up, 0.0f));
			result.setRow(2u, Vector4(direction, 0.0f));

			return result;
		}

		static Matrix4 view[6] = {
			lookAt(Vector3(-1.0f,  0.0f,  0.0f), Vector3(0.0f,  1.0f,  0.0f)),
			lookAt(Vector3(1.0f,  0.0f,  0.0f), Vector3(0.0f,  1.0f,  0.0f)),
			lookAt(Vector3(0.0f, -1.0f,  0.0f), Vector3(0.0f,  0.0f, -1.0f)),
			lookAt(Vector3(0.0f,  1.0f,  0.0f), Vector3(0.0f,  0.0f,  1.0f)),
			lookAt(Vector3(0.0f,  0.0f, -1.0f), Vector3(0.0f,  1.0f,  0.0f)),
			lookAt(Vector3(0.0f,  0.0f,  1.0f), Vector3(0.0f,  1.0f,  0.0f)),
		};

		void EnvMap::setFilter(DeviceContext* context, std::string type) {
			shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("SkyVS", "Sky" + type + "FS");

			if (program)
				context->bindProgram(program.get());
		}

		/* IBL Diffuse Irradiance */
		void EnvMap::faceIrradiance(DeviceContext* context, Sky* sky, uint32_t face) {
			context->bindFramebuffer(irradianceFaces[face].get());

			sky->RenderSkyboxEnvMapBox(context);
		}

		/* IBL Specular Convolution */
		void EnvMap::faceConvolution(DeviceContext* context, Sky* sky, CubemapFace cubemapFace, GlobalShaderData globalShaderData) {
			for (uint32_t m = 0u; m < cubemapMips; ++m) {
				globalShaderData.ViewportSize_ViewportScale = Vector4(globalShaderData.ViewportSize_ViewportScale.x, (float)m / (float)cubemapMips, 0.0f, 0.0f);

				context->updateGlobalConstantData(&globalShaderData, sizeof(globalShaderData));
				context->bindFramebuffer(cubemapFace.mips[m].get());

				sky->RenderSkyboxEnvMapBox(context);
			}
		}

		// full re-render in a single frame
		void EnvMap::fullUpdate(DeviceContext* context) {
			SceneManager* sceneManager = visualEngine->getSceneManager();
			Sky* sky = sceneManager->getSky();
			GlobalShaderData globalShaderData = sceneManager->writeGlobalShaderData();

			Texture* texture = intermediateOutdoorTexture.getTexture().get();
			uint32_t width = texture->getWidth();
			uint32_t height = texture->getHeight();

			globalShaderData.ViewportSize_ViewportScale = Vector4(width, height, 0.0f, 0.0f);

			RenderCamera cam;
			cam.setProjectionPerspective(MATH_PI / 2.0f, 1.0f, 0.1f, 10.0f);

			if (sky->getUseHDRI()) {
				sky->PrepareSkyboxEnvMapEqui(context);

				for (uint32_t i = 0u; i < 6u; ++i) {
					cam.setViewMatrix(view[i]);

					globalShaderData.setCamera(cam);

					context->updateGlobalConstantData(&globalShaderData, sizeof(globalShaderData));
					context->bindFramebuffer(intermediateOutFaces[i].get());

					sky->RenderSkyboxEnvMapBox(context);
				}
			}
			else {
				for (uint32_t i = 0u; i < 6u; ++i) {
					context->bindFramebuffer(intermediateOutFaces[i].get());

					sky->RenderSkyboxEnvMapCube(context, i, outdoorCubemapSize);
				}
			}

			// TODO: Find some other way to copy faces, this is expensive. May not even be required.
			/*for (uint32_t i = 0u; i < 6u; ++i) {
				context->copyFramebuffer(intermediateOutFaces[i].get(), outdoorTexture.getTexture().get());
			}*/

			intermediateOutdoorTexture.getTexture().get()->generateMipmaps();

			context->bindTexture(0u, texture, SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Wrap));

			/* Irradiance - Diffuse */
			setFilter(context, "Irradiance");

			for (uint32_t i = 0u; i < 6u; ++i) {
				cam.setViewMatrix(view[i]);

				globalShaderData.setCamera(cam);

				context->updateGlobalConstantData(&globalShaderData, sizeof(globalShaderData));

				faceIrradiance(context, sky, i);
			}

			/* Convolution - Specular */
			setFilter(context, "Convolution");

			for (uint32_t i = 0u; i < 6u; ++i) {
				cam.setViewMatrix(view[i]);

				globalShaderData.setCamera(cam);

				faceConvolution(context, sky, outFaces[i], globalShaderData);
			}
		}

		void EnvMap::onDeviceRestored()
		{
		}


	}
}
