#include "stdafx.h"
#include "SceneManager.h"

#include "VisualEngine.h"
#include "RenderQueue.h"
#include "Material.h"
#include "ShaderManager.h"
#include "MeshInstancer.h"

#include "SpatialHashedScene.h"
#include "AdornRender.h"
#include "VertexStreamer.h"
#include "Vertex.h"
#include "LightObject.h"

#include "Bloom.h"
#include "FastCluster.h"
#include "Sky.h"
#include "SSAO.h"
#include "EnvMap.h"
#include "ScreenSpaceEffect.h"
#include "TextureManager.h"
#include "MaterialGenerator.h"

#include "GfxCore/Device.h"
#include "GfxCore/Framebuffer.h"

#include "GfxBase/FrameRateManager.h"
#include "GfxBase/RenderStats.h"

#include "EmitterShared.h"

#include "rbx/Profiler.h"

LOGGROUP(Graphics)

//FASTFLAGVARIABLE(GlowEnabled, true)

FASTFLAG(RenderVR)

FASTFLAG(RenderUIAs3DInVR)

namespace RBX {
	namespace Graphics {

		/*static void renderObjectsImpl(DeviceContext* context, const RenderQueueGroup& group, RenderPassStats& stats, const char* dbgname) {
			float matrixData[256 * 12];

			const Technique* cachedTechnique = nullptr;
			const void* cachedMatrixData = nullptr;

			PIX_SCOPE(context, "Render: %s", dbgname);

			for (size_t i = 0u; i < group.size(); ++i) {
				const RenderOperation& rop = group[i];

				if (cachedTechnique != rop.technique) {
					cachedTechnique = rop.technique;
					cachedMatrixData = nullptr;

					rop.technique->apply(context);

					stats.passChanges++;
				}

				if (rop.renderable) {
					uint32_t matrixCount = rop.renderable->getWorldTransforms(matrixData, &cachedMatrixData);

					if (matrixCount) {
						float matrix[16];

						memcpy(matrix, matrixData, 3 * 4 * 4);

						InstancedModel newMatrix;
						newMatrix.Model = Matrix4(matrix);
					}
				}

				context->draw(*rop.geometry);

				stats.batches++;
				stats.vertices += rop.geometry->getVertexCount();
				stats.faces += rop.geometry->getVertexCount() / 3u;
			}
		}*/

		/*static void renderObjects(DeviceContext* context, RenderQueueGroup& group, RenderQueueGroup::SortMode sortMode, RenderPassStats& stats, const char* dbgname) {
			if (group.size() == 0u)
				return;

			RBXPROFILER_SCOPE("Render", "$Pass");
			RBXPROFILER_LABELF("Render", "%s (%d)", dbgname, group.size());

			RBXPROFILER_SCOPE("GPU", "$Pass");
			RBXPROFILER_LABELF("GPU", "%s (%d)", dbgname, group.size());

			{
				RBXPROFILER_SCOPE("Render", "sort");

				group.sort(sortMode);
			}

			{
				RBXPROFILER_SCOPE("Render", "draw");

				renderObjectsImpl(context, group, stats, dbgname);
			}
		}*/

		static void renderInstancedObjectsImpl(DeviceContext* context, const RenderQueueGroup& group, RenderPassStats& stats, const char* dbgname) {
			const Technique* cachedTechnique = nullptr;

			PIX_SCOPE(context, "Render: %s", dbgname);

			for (size_t i = 0u; i < group.size(); ++i) {
				const RenderOperation& rop = group[i];
				size_t instanceCount = rop.models->Models.size();

				if (cachedTechnique != rop.technique) {
					cachedTechnique = rop.technique;

					cachedTechnique->apply(context);

					stats.passChanges++;
				}

				context->updateInstancedModels(rop.models->Models.data(), instanceCount * sizeof(InstancedModel));
				context->drawInstanced(rop.geometry, instanceCount);

				stats.batches++;
				stats.vertices += rop.geometry.getVertexCount() * instanceCount;
				stats.faces += rop.geometry.getVertexCount() / 3u * instanceCount;
			}
		}

		static void renderInstancedObjects(DeviceContext* context, RenderQueueGroup& group, RenderQueueGroup::SortMode sortMode, RenderPassStats& stats, const char* dbgname) {
			if (group.size() == 0u)
				return;

			RBXPROFILER_SCOPE("RenderInstanced", "$Pass");
			RBXPROFILER_LABELF("RenderInstanced", "%s (%d)", dbgname, group.size());

			RBXPROFILER_SCOPE("GPU", "$Pass");
			RBXPROFILER_LABELF("GPU", "%s (%d)", dbgname, group.size());

			{
				RBXPROFILER_SCOPE("RenderInstanced", "sort");

				group.sort(sortMode);
			}

			{
				RBXPROFILER_SCOPE("RenderInstanced", "draw");

				renderInstancedObjectsImpl(context, group, stats, dbgname);
			}
		}

		static std::shared_ptr<Geometry> createFullscreenTriangle(Device* device) {
			std::array<BasicVertex, 3u> vertices = {
				BasicVertex(-1.0f,  1.0f, 0.0f),
				BasicVertex(3.0f,  1.0f, 0.0f),
				BasicVertex(-1.0f, -3.0f, 0.0f),
			};

			/* 1 - 2 */
			/* - - - */
			/* 3 - - */

			std::shared_ptr<VertexBuffer> vertexBuffer = device->createVertexBuffer(sizeof(BasicVertex), vertices.size(), GeometryBuffer::Usage_Static);

			vertexBuffer->upload(0u, vertices.data(), vertices.size() * sizeof(BasicVertex));

			return device->createGeometry(
				device->createVertexLayout(BasicVertex::getVertexLayout()),
				vertexBuffer,
				std::shared_ptr<IndexBuffer>());
		}

		class Blur {
		public:

			Blur(VisualEngine* visualEngine)
				: visualEngine(visualEngine)
				, blurError(false)
				, blurStrength(5)
			{
			}

			bool valid() const { return data != nullptr; }

			void update(unsigned width, unsigned height, const SceneManager::PostProcessSettings& pps)
			{
				blurStrength = pps.blurIntensity;

				if (data && !blurNeeded()) {
					data.reset();
				}
				else {
					if (!blurError && blurNeeded() && (!data.get() || data->width != width || data->height != height)) {
						try {
							data.reset(createData(width, height));
						}
						catch (const RBX::base_exception&) {
							blurError = true;
							data.reset();
						}
					}
				}
			}

			void render(DeviceContext* context, Framebuffer* bufferToBlur, Texture* inTexture) {
				if (!data) return;

				//ScreenSpaceEffect::renderBlur(context, visualEngine, bufferToBlur, data->intermediateFB[0].get(), inTexture, data->intermediateTex[0].get(), blurStrength);
			}

			void render(DeviceContext* context, Framebuffer* bufferToBlur) {
				if (!data) return;

				context->copyFramebuffer(bufferToBlur, data->intermediateTex[0].get());
				//ScreenSpaceEffect::renderBlur(context, visualEngine, bufferToBlur, data->intermediateFB[1].get(), data->intermediateTex[0].get(), data->intermediateTex[1].get(), blurStrength);
			}

		private:
			struct Data {
				std::shared_ptr<Texture> intermediateTex[2];
				std::shared_ptr<Framebuffer> intermediateFB[2];
				unsigned width;
				unsigned height;
			};

			Data* createData(unsigned width, unsigned height) {
				auto* data = new Data();

				Device* device = visualEngine->getDevice();

				data->intermediateTex[0u] = device->createTexture(Texture::Type_2D, Texture::Format_RGBA16f, width, height, 1u, 1u, Texture::Usage_Colorbuffer);
				data->intermediateFB[0u] = device->createFramebuffer(data->intermediateTex[0u]->getRenderbuffer(0u, 0u));

				data->intermediateTex[1u] = device->createTexture(Texture::Type_2D, Texture::Format_RGBA16f, width, height, 1u, 1u, Texture::Usage_Colorbuffer);
				data->intermediateFB[1u] = device->createFramebuffer(data->intermediateTex[1u]->getRenderbuffer(0u, 0u));

				data->width = width;
				data->height = height;

				return data;
			}

			bool blurNeeded() const {
				return blurStrength > FLT_EPSILON;
			}

			std::unique_ptr<Data> data;
			bool blurError;
			VisualEngine* visualEngine;

			float blurStrength;
		};

		class ImageProcess {
		public:

			ImageProcess(VisualEngine* visualEngine)
				: visualEngine(visualEngine)
				, grayscaleLevel(1.0f)
				, brightness(1.0f)
				, contrast(1.0f)
				, tintColor(Color3(1.0f, 1.0f, 1.0f))
				, tonemapper(ACES)
			{
			}

			void update(const SceneManager::PostProcessSettings& pps) {
				brightness = pps.brightness;
				contrast = pps.contrast;
				grayscaleLevel = pps.grayscaleLevel;
				tintColor = pps.tintColor;
				tonemapper = pps.tonemapper;
			}

			void render(DeviceContext* context, std::shared_ptr<Texture> source, Bloom* bloom, GlobalProcessingData globalProcessingData) {
				std::string nameFS = "Tonemap" + getTonemapper(tonemapper);

				float bloomIntensity = 0.0f;

				if (bloom) {
					nameFS += "Bloom";

					bloomIntensity = bloom->getIntensity() / 10.0f;
				}

				nameFS += "FS";

				uint32_t width = source->getWidth();
				uint32_t height = source->getHeight();

				globalProcessingData.TextureSize_ViewportScale = Vector4(width, height, 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height));
				globalProcessingData.Parameters1 = Vector4(brightness - 1.0f, contrast, grayscaleLevel, bloomIntensity);
				globalProcessingData.Parameters2 = Vector4(tintColor.r, tintColor.g, tintColor.b, 0.0f);
				context->updateGlobalProcessingData(&globalProcessingData, sizeof(globalProcessingData));
				
				if (ShaderProgram* program = ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "PassThroughVS", nameFS.c_str(), BlendState::Mode_None, width, height)) {
					if (bloom) {
						std::vector<Texture::TextureStage> textures;

						textures.push_back(Texture::TextureStage(0u, source));
						textures.push_back(Texture::TextureStage(1u, bloom->getTexture()));

						context->bindTextures(textures);
					}
					else
						context->bindTexture(0u, source);

					ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
				}
			}

		private:
			std::string getTonemapper(TonemapperMode mode) {
				switch (mode) {
				case(REINHARD):
					return "Reinhard";
				case(ACES):
					return "ACES";
				case(PBR_NEUTRAL):
					return "PBRNeutral";
				case(AGX):
					return "AgX";
				default:
					return "";
				}
			}

			bool processingNeeded() {
				return tintColor != Color3::one() || brightness != 1.0f || contrast != 1.0f || grayscaleLevel != 1.0f;
			}

			VisualEngine* visualEngine;

			float brightness;
			float contrast;
			float grayscaleLevel;
			Color3 tintColor;
			TonemapperMode tonemapper;
		};

		/*class ShadowMap
		{
		public:
			ShadowMap(VisualEngine* visualEngine, Texture::Format format, int size)
			{
				Device* device = visualEngine->getDevice();

				shadowMap[0] = device->createTexture(Texture::Type_2D, format, size, size, 1, 1, Texture::Usage_Colorbuffer);
				shadowMap[1] = device->createTexture(Texture::Type_2D, format, size, size, 1, 1, Texture::Usage_Colorbuffer);

				shadowMapFB[0] = device->createFramebuffer(shadowMap[0]->getRenderbuffer(0, 0));
				shadowMapFB[1] = device->createFramebuffer(shadowMap[1]->getRenderbuffer(0, 0));

				std::shared_ptr<Renderbuffer> shadowDepth = device->createRenderbuffer(Texture::Format_D32f, size, size, 1);

				shadowFB = device->createFramebuffer(shadowMap[0]->getRenderbuffer(0, 0), shadowDepth);
			}

			const std::shared_ptr<Texture>& getTexture() const
			{
				return shadowMap[0];
			}

			std::shared_ptr<Framebuffer> shadowFB;

			std::shared_ptr<Texture> shadowMap[2];
			std::shared_ptr<Framebuffer> shadowMapFB[2];
		};*/

		SceneManager::SceneManager(VisualEngine* visualEngine)
			: Resource(visualEngine->getDevice())
			, visualEngine(visualEngine)
			, sqMinPartDistance(FLT_MAX)
			, skyEnabled(true)
			, clearColor(Color4::zero())
			, postProcessSettings(PostProcessSettings())
		{
			Device* device = visualEngine->getDevice();

			spatialHashedScene.reset(new SpatialHashedScene());
			renderQueue.reset(new RenderQueue());

			fullscreenTriangle.reset(new GeometryBatch(createFullscreenTriangle(device), Geometry::Primitive_TriangleStrip, 3u, 0u));

			sky.reset(new Sky(visualEngine));
			//ssao.reset(new SSAO(visualEngine));
			bloom.reset(new Bloom(visualEngine));
			blur.reset(new Blur(visualEngine));
			envMap.reset(new EnvMap(visualEngine));
			imageProcess.reset(new ImageProcess(visualEngine));

			/*shadowRenderQueue.reset(new RenderQueue());

			shadowMask = visualEngine->getTextureManager()->load(ContentId("rbxasset://textures/shadowmask.png"), TextureManager::Fallback_Black);

			try
			{
#if !defined(RBX_PLATFORM_IOS) && !defined(__ANDROID__)
				shadowMapTexelSize = 0.2f;

				shadowMaps[0].reset(new ShadowMap(visualEngine, Texture::Format_RGBA8, 256));
				shadowMaps[1].reset(new ShadowMap(visualEngine, Texture::Format_RGBA8, 512));
				shadowMaps[2].reset(new ShadowMap(visualEngine, Texture::Format_RGBA8, 1024));
#else
				shadowMapTexelSize = 0.3f;

				shadowMaps[0].reset(new ShadowMap(visualEngine, Texture::Format_RGBA8, 128));
				shadowMaps[1].reset(new ShadowMap(visualEngine, Texture::Format_RGBA8, 256));
#endif
			}
			catch (RBX::base_exception& e)
			{
				FASTLOGS(FLog::Graphics, "Error creating shadow maps: %s", e.what());
			}

			// Create dummy GBuffer textures so that materials can have passes that reference them
			gbufferColor = shared_ptr<Texture>();
			gbufferDepth = shared_ptr<Texture>();
			shadowMapTexture = shared_ptr<Texture>();*/

			shadowMapAtlas = std::shared_ptr<Texture>();
			shadowMapArray = std::shared_ptr<Texture>();
			ambientOcclusion = std::shared_ptr<Texture>();
		}

		SceneManager::~SceneManager()
		{
		}

		void SceneManager::computeMinimumSqDistance(const RenderCamera& camera) {
			sqMinDistanceCenter = pointOfInterest;
			sqMinPartDistance = FLT_MAX;

			std::vector<CullableSceneNode*> nodes;
			spatialHashedScene->queryFrustumOrdered(nodes, camera, pointOfInterest, visualEngine->getFrameRateManager());

			// Required to call SceneManager::processSqPartDistance
			for (CullableSceneNode* node : nodes)
				node->updateIsCulledByFRM();
		}

		void SceneManager::renderScene(DeviceContext* context, Framebuffer* mainFramebuffer, const RenderCamera& camera, uint32_t viewWidth, uint32_t viewHeight) {
			RBXPROFILER_SCOPE("GPU", "Scene");
			RBXPROFILER_SCOPE("Render", "Scene");

			renderBegin(context, camera, viewWidth, viewHeight);
			renderView(context, mainFramebuffer, camera, viewWidth, viewHeight);
			renderEnd(context);
		}

		void SceneManager::renderBegin(DeviceContext* context, const RenderCamera& camera, uint32_t viewWidth, uint32_t viewHeight) {
			FrameRateManager* frm = visualEngine->getFrameRateManager();
			RenderStats* stats = visualEngine->getRenderStats();

			updateMain(viewWidth, viewHeight);
			updateMSAA(viewWidth, viewHeight);

			// prepare UI
			//visualEngine->getVertexStreamer()->renderPrepare();

			context->bindSamplers();
			context->setGlobalConstantData();

			/* Environment Map */
			{
				RBXPROFILER_SCOPE("Render", "Environment Map");
				RBXPROFILER_SCOPE("GPU", "Environment Map");

				envMap->update(context, curTimeOfDay);
			}

			// Prepare for stats gathering
			stats->passTotal = stats->passScene = stats->passShadow = stats->passUI = stats->pass3DAdorns = RenderPassStats();

			sqMinDistanceCenter = pointOfInterest;
			sqMinPartDistance = FLT_MAX;

			// Get all visible ROPs
			renderNodes.clear();
			spatialHashedScene->getAllNodes();//queryFrustumOrdered(renderNodes, camera, pointOfInterest, frm);

			{
				RBXPROFILER_SCOPE("Render", "updateRenderQueue");

				std::vector<LightObject*> lights;
				for (CullableSceneNode* node : renderNodes) {
					LightObject* lobj = static_cast<LightObject*>(node);

					// Cull lights based on brightness, range and color
					if (lobj)
						if (lobj->getType() != LightObject::Type_None && lobj->getBrightness() > FLT_EPSILON && lobj->getRange() > FLT_EPSILON && !lobj->getColor().isZero())
							lights.push_back(lobj);
					//else
						//node->updateRenderQueue(*renderQueue, camera, RenderQueue::Pass_Default);
				}

				// TODO: Assign light counter to a temporary variable in globals for now
				size_t lightCount = globalLightList.populateList(lights);

				context->updateGlobalLightList(globalLightList.LightList.data(), lightCount);
			}

			visualEngine->getMeshInstancer()->updateRenderQueue(*renderQueue, camera);

			StandardOut::singleton()->printf(MESSAGE_INFO, "Opaque renderables: %i", renderQueue->getGroup(0u).size());

			// Flush particle vertex buffer; it's being filled in particle emitter updateRenderQueue
			//visualEngine->getEmitterSharedState()->flush();

			bloom->update(viewWidth, viewHeight, postProcessSettings.bloomIntensity, postProcessSettings.bloomSize);
			//blur->update(viewWidth, viewHeight, postProcessSettings);
			imageProcess->update(postProcessSettings);

			GlobalMaterialData materialData = visualEngine->getMaterialGenerator()->getMaterialData();
			context->updateGlobalMaterialData(&materialData, sizeof(materialData));

			// Render shadow map
			/*ShadowMap* shadowMap = pickShadowMap(frm->GetQualityLevel());

			if (shadowMap)
			{
				renderShadowMap(context, shadowMap);

				shadowMapTexture.updateAllRefs(shadowMap->getTexture());
			}
			else
			{
				globalShaderData.OutlineBrightness_ShadowInfo.w = 0;

				shadowMapTexture.updateAllRefs(std::shared_ptr<Texture>());
			}*/
		}

		void SceneManager::renderView(DeviceContext* context, Framebuffer* mainFramebuffer, const RenderCamera& camera, uint32_t viewWidth, uint32_t viewHeight) {
			RenderStats* stats = visualEngine->getRenderStats();

			// Set up main camera
			globalShaderData.setCamera(camera);

			context->updateGlobalConstantData(&globalShaderData, sizeof(globalShaderData));

			/* Clear */
			{
				RBXPROFILER_SCOPE("Render", "Clear");
				RBXPROFILER_SCOPE("GPU", "Clear");

				if (msaa) {
					context->bindFramebuffer(msaa->framebuffer.get());
					context->clearFramebuffer(DeviceContext::Buffer_Color | DeviceContext::Buffer_Depth | DeviceContext::Buffer_Stencil, &clearColor.r, 0.0f, 0u);
				}
				else {
					context->bindFramebuffer(main->framebuffer.get());
					context->clearFramebuffer(DeviceContext::Buffer_Color | DeviceContext::Buffer_Depth | DeviceContext::Buffer_Stencil, &clearColor.r, 0.0f, 0u);
				}
			}

			context->setGlobalMaterialData();
			context->setGlobalLightList();
			context->setInstancedModels();

			/* Opaque Geometry */
			{
				RBXPROFILER_SCOPE("Render", "Opaque Geometry");
				RBXPROFILER_SCOPE("GPU", "Opaque Geometry");

				//renderInstancedObjects(context, renderQueue->getGroup(RenderQueue::Flag_Opaque), RenderQueueGroup::Sort_Material, stats->passScene, "Id_Opaque");
			}

			/* Opaque Decals */
			/*{
				RBXPROFILER_SCOPE("Render", "Opaque Decals");
				RBXPROFILER_SCOPE("GPU", "Opaque Decals");

				renderObjects(context, renderQueue->getGroup(RenderQueue::Id_OpaqueDecals), RenderQueueGroup::Sort_Distance, stats->passScene, "Id_OpaqueDecals");
			}*/

			/* Sky */
			if (skyEnabled) {
				RBXPROFILER_SCOPE("Render", "Sky");
				RBXPROFILER_SCOPE("GPU", "Sky");

				sky->render(context, camera, envMap->getOutdoorTexture());
			}

			/* Transparent Geometry */
			/*{
				RBXPROFILER_SCOPE("Render", "Transparent Geometry");
				RBXPROFILER_SCOPE("GPU", "Transparent Geometry");

				renderObjects(context, renderQueue->getGroup(RenderQueue::Id_TransparentUnsorted), RenderQueueGroup::Sort_Material, stats->passScene, "Id_TransparentUnsorted");
				renderObjects(context, renderQueue->getGroup(RenderQueue::Id_Transparent), RenderQueueGroup::Sort_Distance, stats->passScene, "Id_Transparent");
			}*/

			/* Transparent Decals */
			/*{
				RBXPROFILER_SCOPE("Render", "Transparent Decals");
				RBXPROFILER_SCOPE("GPU", "Transparent Decals");

				renderObjects(context, renderQueue->getGroup(RenderQueue::Id_TransparentDecals), RenderQueueGroup::Sort_Material, stats->passScene, "Id_TransparentDecals");
			}*/

			/* World UI */
			/*{
				RBXPROFILER_SCOPE("Render", "3D UI");
				RBXPROFILER_SCOPE("GPU", "3D UI");

				visualEngine->getVertexStreamer()->render3D(context, camera, stats->passUI);

				for (int renderIndex = 0; renderIndex <= Adorn::maximumZIndex; ++renderIndex)
				{
					visualEngine->getAdorn()->renderNoDepth(context, stats->pass3DAdorns, renderIndex);
					visualEngine->getVertexStreamer()->render3DNoDepth(context, camera, stats->pass3DAdorns, renderIndex);
				}
			}*/

			/* 3D Adornments */
			/*{
				RBXPROFILER_SCOPE("Render", "Adorns");
				RBXPROFILER_SCOPE("GPU", "Adorns");

				visualEngine->getAdorn()->render(context, stats->pass3DAdorns);
			}*/

			/* MSAA Resolve */
			if (msaa) {
				RBXPROFILER_SCOPE("Render", "MSAA Resolve");
				RBXPROFILER_SCOPE("GPU", "MSAA Resolve");

				context->resolveFramebuffer(msaa->framebuffer.get(), main->framebuffer.get(), DeviceContext::Buffer_Color);
			}

			context->setGlobalProcessingData();

			/* Blur */
			/*if (blur) {
				RBXPROFILER_SCOPE("Render", "Blur");
				RBXPROFILER_SCOPE("GPU", "Blur");

				context->bindFramebuffer(main->mainFB.get());

				blur->render(context, main->mainFB.get());
			}*/

			/* Bloom */
			/*if (bloom) {
				RBXPROFILER_SCOPE("Render", "Bloom");
				RBXPROFILER_SCOPE("GPU", "Bloom");

				bloom->render(context, main->mainColor, globalProcessingData);
			}*/

			/* Tonemapping */
			{
				RBXPROFILER_SCOPE("Render", "Tonemapping");
				RBXPROFILER_SCOPE("GPU", "Tonemapping");

				context->bindFramebuffer(mainFramebuffer);

				imageProcess->render(context, main->color, nullptr /*bloom.get()*/, globalProcessingData);
			}

			/* Screen UI */
			/*{
				RBXPROFILER_SCOPE("Render", "2D UI");
				RBXPROFILER_SCOPE("GPU", "2D UI");

				if (!FFlag::RenderUIAs3DInVR && visualEngine->getDevice()->getVR())
					visualEngine->getVertexStreamer()->render2DVR(context, viewWidth, viewHeight, stats->passUI);
				else
					visualEngine->getVertexStreamer()->render2D(context, viewWidth, viewHeight, stats->passUI);
			}*/
		}

		void SceneManager::renderEnd(DeviceContext* context) {
			RenderStats* stats = visualEngine->getRenderStats();

			renderQueue->clear();

			visualEngine->getVertexStreamer()->renderFinish();

			// Finalize stats
			stats->passTotal = stats->passScene + stats->passShadow + stats->passUI + stats->pass3DAdorns;
		}

		/*SceneManager::ShadowValues SceneManager::unsetAndGetShadowValues(DeviceContext* context)
		{
			ShadowValues values = { globalShaderData.OutlineBrightness_ShadowInfo.w };

			globalShaderData.OutlineBrightness_ShadowInfo.w = 0;

			context->updateGlobalConstants(&globalShaderData, sizeof(globalShaderData));

			return values;
		}

		void SceneManager::restoreShadows(DeviceContext* context, const ShadowValues& shadowValues)
		{
			globalShaderData.OutlineBrightness_ShadowInfo.w = shadowValues.intensity;

			context->updateGlobalConstants(&globalShaderData, sizeof(globalShaderData));
		}*/

		void SceneManager::setPointOfInterest(const Vector3& poi) {
			pointOfInterest = poi;
		}

		void SceneManager::setSkyEnabled(bool value) {
			skyEnabled = value;
		}

		void SceneManager::setClearColor(const Color4& value) {
			clearColor = value;
		}

		void SceneManager::setFog(const Color3& color, float density, float sunInfluence, bool usesSkybox, bool affectsSkybox) {
			globalShaderData.FogColor_Density = Color4(color, density);
			globalShaderData.FogParams = Vector4(sunInfluence, static_cast<float>(usesSkybox), static_cast<float>(affectsSkybox), 0.0f);
		}

		void SceneManager::setLighting(const Color3& ambient, const Color3& outdoorAmbient, const Vector3& keyDirection, const Color3& keyColor) {
			globalShaderData.AmbientColor_EnvDiffuse = Color4(ambient, globalShaderData.AmbientColor_EnvDiffuse.w);
			globalShaderData.OutdoorAmbientColor_EnvSpecular = Color4(outdoorAmbient, globalShaderData.OutdoorAmbientColor_EnvSpecular.w);
			globalShaderData.KeyColor_KeyShadowDistance = Color4(keyColor, globalShaderData.KeyColor_KeyShadowDistance.w);
			globalShaderData.KeyDirection_unused = Vector4(keyDirection, globalShaderData.KeyDirection_unused.w);
		}

		void SceneManager::setPostProcess(float brightness, float contrast, float grayscaleLevel, const Color3& tintColor, TonemapperMode tonemapper, float blurIntensity, float bloomIntensity, uint32_t bloomSize) {
			postProcessSettings.brightness = brightness;
			postProcessSettings.contrast = contrast;
			postProcessSettings.grayscaleLevel = grayscaleLevel;
			postProcessSettings.tintColor = tintColor;
			postProcessSettings.tonemapper = tonemapper;

			postProcessSettings.blurIntensity = blurIntensity;

			postProcessSettings.bloomIntensity = bloomIntensity;
			postProcessSettings.bloomSize = bloomSize;
		}

		void SceneManager::setProcessing(float exposure, float gamma) {
			globalProcessingData.Exposure_Gamma_InverseGamma_Unused = Vector4(exposure, gamma, 1.0f / gamma, 0.0f);
		}

		void SceneManager::updateMain(uint32_t width, uint32_t height) {
			if (!main || (main->framebuffer->getWidth() != width || main->framebuffer->getHeight() != height)) {
				Device* device = visualEngine->getDevice();

				try {
					main.reset(new RenderTarget());

					main->color = device->createTexture(Texture::Type_2D, Texture::Format_RGBA16f, width, height, 1u, 1u, Texture::Usage_Colortexture);
					main->depth = device->createTexture(Texture::Type_2D, Texture::Format_D32f, width, height, 1u, 1u, Texture::Usage_Depthtexture);
					
					main->framebuffer = device->createFramebuffer(main->color->getRenderbuffer(0u, 0u), main->depth->getRenderbuffer(0u, 0u));
				}
				catch (RBX::base_exception& e) {
					main.reset();

					throw RBX::runtime_error("Error initializing main: %s", e.what());
				}
			}
		}

		void SceneManager::updateMSAA(uint32_t width, uint32_t height) {
			if (device->getCaps().maxSamples <= 1u) {
				if (msaa)
					msaa.reset();

				return;
			}

			if (!msaa || msaa->framebuffer->getWidth() != width || msaa->framebuffer->getHeight() != height) {
				try {
					msaa.reset(new RenderTarget());

					msaa->color = device->createTexture(Texture::Type_2DMS, Texture::Format_RGBA16f, width, height, 1u, 1u, Texture::Usage_Colorbuffer);
					msaa->depth = device->createTexture(Texture::Type_2DMS, Texture::Format_D32f, width, height, 1u, 1u, Texture::Usage_Depthbuffer);

					msaa->framebuffer = device->createFramebuffer(msaa->color->getRenderbuffer(0u, 0u), msaa->depth->getRenderbuffer(0u, 0u));
				}
				catch (RBX::base_exception& e) {
					msaa.reset();

					throw RBX::runtime_error("Error initializing MSAA: %s", e.what());
				}
			}
		}

		/*void SceneManager::updateGBuffer(unsigned width, unsigned height)
		{
			if (gbufferError || msaa)
			{
				gbuffer.reset();
				return;
			}

			FrameRateManager* frm = visualEngine->getFrameRateManager();
			if (!frm->getGBufferSetting())
			{
				// gbuffer is not required, kill it and bail out
				gbuffer.reset();
				return;
			}

			if (!gbuffer || (gbuffer->mainColor->getWidth() != width || gbuffer->mainColor->getHeight() != height))
			{
				gbuffer.reset();
				gbuffer.reset(new GBuffer());

				Device* device = visualEngine->getDevice();

				try {
					std::shared_ptr<Renderbuffer> mainDepth = device->createRenderbuffer(Texture::Format_D32f, width, height, 1);

					gbuffer->mainColor = device->createTexture(Texture::Type_2D, Texture::Format_RGBA16F, width, height, 1, 1, Texture::Usage_Colorbuffer);
					gbuffer->mainFB = device->createFramebuffer(gbuffer->mainColor->getRenderbuffer(0, 0), mainDepth);

					gbuffer->gbufferColor = device->createTexture(Texture::Type_2D, Texture::Format_RGBA16F, width, height, 1, 1, Texture::Usage_Colorbuffer);
					gbuffer->gbufferColorFB = device->createFramebuffer(gbuffer->gbufferColor->getRenderbuffer(0, 0), mainDepth);

					gbuffer->gbufferDepth = device->createTexture(Texture::Type_2D, Texture::Format_RGBA16F, width, height, 1, 1, Texture::Usage_Colorbuffer);
					gbuffer->gbufferDepthFB = device->createFramebuffer(gbuffer->gbufferDepth->getRenderbuffer(0, 0));

					std::vector<std::shared_ptr<Renderbuffer> > gbufferTextures;
					gbufferTextures.push_back(gbuffer->gbufferColor->getRenderbuffer(0, 0));
					gbufferTextures.push_back(gbuffer->gbufferDepth->getRenderbuffer(0, 0));

					gbuffer->gbufferFB = device->createFramebuffer(gbufferTextures, mainDepth);
				}
				catch (RBX::base_exception& e)
				{
					FASTLOGS(FLog::Graphics, "Error initializing GBuffer: %s", e.what());

					gbuffer.reset();
					gbufferError = true; // and do not try any further
				}
			}
		}

		void SceneManager::resolveGBuffer(DeviceContext* context, Texture* texture)
		{
			RBXASSERT(gbuffer);
			PIX_SCOPE(context, "resolveGBuffer");
			RBXPROFILER_SCOPE("Render", "resolveGBuffer");
			RBXPROFILER_SCOPE("GPU", "resolveGBuffer");

			float width = texture->getWidth();
			float height = texture->getHeight();

			const float textureSize[] = { width, height, 1 / width, 1 / height };

			context->setRasterizerState(RasterizerState::Cull_None);
			context->setBlendState(BlendState::Mode_None);
			context->setDepthState(DepthState(DepthState::Function_Always, false));

			static const std::string vsName = "GBufferResolveVS";
			static const std::string fsName = "GBufferResolveFS";

			if (std::shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram(vsName, fsName))
			{
				context->bindProgram(program.get());
				context->bindTexture(0, texture, SamplerState(SamplerState::Filter_Point, SamplerState::Address_Clamp));
				context->setConstant(program->getConstantHandle("TextureSize"), textureSize, 1);

				context->draw(getFullscreenTriangle());
			}
		}

		void SceneManager::renderShadowMap(DeviceContext* context, ShadowMap* shadowMap)
		{
			RBXPROFILER_SCOPE("Render", "renderShadowMap");
			RBXPROFILER_SCOPE("GPU", "ShadowMap");

			int shadowMapSize = shadowMap->getTexture()->getWidth();
			float shadowRadius = shadowMapSize * shadowMapTexelSize / 2.f;
			float shadowDepthRadius = 64.f;
			Vector3 shadowCenter = pointOfInterest;

			RenderCamera shadowCamera = getShadowCamera(shadowCenter, shadowMapSize, shadowRadius, shadowDepthRadius);

			renderNodes.clear();
			spatialHashedScene->querySphere(renderNodes, shadowCenter, std::max(shadowRadius, shadowDepthRadius), CullableSceneNode::Flags_ShadowCaster);

			{
				RBXPROFILER_SCOPE("Render", "updateRenderQueue");

				shadowRenderQueue->clear();

				for (CullableSceneNode* node : renderNodes)
					node->updateRenderQueue(*shadowRenderQueue, shadowCamera, RenderQueue::Pass_Shadows);
			}

			globalShaderData.setCamera(shadowCamera);

			float clipSign = visualEngine->getDevice()->getCaps().requiresRenderTargetFlipping ? 1 : -1;
			Matrix4 clipToUv = Matrix4::translation(0.5f, 0.5f, 0) * Matrix4::scale(0.5f, clipSign * 0.5f, 1);
			Matrix4 textureMatrix = clipToUv * shadowCamera.getViewProjectionMatrix();

			globalShaderData.ShadowMatrix0 = textureMatrix.row(0);
			globalShaderData.ShadowMatrix1 = textureMatrix.row(1);
			globalShaderData.ShadowMatrix2 = textureMatrix.row(2);

			context->updateGlobalConstants(&globalShaderData, sizeof(globalShaderData));

			const float clearColor[] = { 1, 0, 0, 0 };

			context->bindFramebuffer(shadowMap->shadowFB.get());
			context->clearFramebuffer(DeviceContext::Buffer_Color | DeviceContext::Buffer_Depth, clearColor, 1.f, 0);

			RenderQueueGroup& group = shadowRenderQueue->getGroup(RenderQueue::Id_OpaqueCasters);

			if (group.size() > 0)
			{
				renderObjects(context, group, RenderQueueGroup::Sort_Material, visualEngine->getRenderStats()->passShadow, "Id_OpaqueCasters");

				blurShadowMap(context, shadowMap);
			}
		}

		RenderCamera SceneManager::getShadowCamera(const Vector3& center, int shadowMapSize, float shadowRadius, float shadowDepthRadius)
		{
			RenderCamera shadowCamera;

			CoordinateFrame cframe;
			cframe.translation = pointOfInterest;
			cframe.lookAt(cframe.translation - globalShaderData.Lamp0Dir.xyz());

			Matrix4 proj = Matrix4::identity();

			proj[0][0] = 1.f / shadowRadius;
			proj[1][1] = 1.f / shadowRadius;
			proj[2][2] = 1.f / shadowDepthRadius;
			proj[2][3] = 0.5f;

			shadowCamera.setViewCFrame(cframe);
			shadowCamera.setProjectionMatrix(proj);

			// Round translation
			int roundThreshold = shadowMapSize / 2;

			Vector4 centerp = shadowCamera.getViewProjectionMatrix() * Vector4(0, 0, 0, 1);
			float centerX = floorf(centerp.x * roundThreshold) / roundThreshold;
			float centerY = floorf(centerp.y * roundThreshold) / roundThreshold;

			proj[0][3] = centerX - centerp.x;
			proj[1][3] = centerY - centerp.y;

			shadowCamera.setProjectionMatrix(proj);

			return shadowCamera;
		}

		void SceneManager::blurShadowMap(DeviceContext* context, ShadowMap* shadowMap)
		{
			RBXPROFILER_SCOPE("Render", "Blur");
			RBXPROFILER_SCOPE("GPU", "Blur");

			int shadowMapSize = shadowMap->getTexture()->getWidth();

			context->bindFramebuffer(shadowMap->shadowMapFB[1].get());

			const std::shared_ptr<Texture>& dummyMask = visualEngine->getTextureManager()->getFallbackTexture(TextureManager::Fallback_White);

			if (ShaderProgram* program = ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "PassThroughVS", "ShadowBlurFS", BlendState::Mode_None, shadowMapSize, shadowMapSize))
			{
				float params[] = { 1.f / shadowMapSize, 0, 0, 0 };
				context->setConstant(program->getConstantHandle("Params1"), params, 1);
				context->bindTexture(0, shadowMap->shadowMap[0].get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));
				context->bindTexture(1, dummyMask.get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

				ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
			}

			context->bindFramebuffer(shadowMap->shadowMapFB[0].get());

			if (ShaderProgram* program = ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "PassThroughVS", "ShadowBlurFS", BlendState::Mode_None, shadowMapSize, shadowMapSize))
			{
				float params[] = { 0, 1.f / shadowMapSize, 0, 0 };
				context->setConstant(program->getConstantHandle("Params1"), params, 1);
				context->bindTexture(0, shadowMap->shadowMap[1].get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));
				context->bindTexture(1, shadowMask.getTexture().get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

				ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
			}
		}*/

		void SceneManager::onDeviceLost()
		{
		}

		/*ShadowMap* SceneManager::pickShadowMap(int qualityLevel) const
		{
			if (shadowMaps[2] && qualityLevel >= 14)
				return shadowMaps[2].get();

			if (shadowMaps[1] && qualityLevel >= 6)
				return shadowMaps[1].get();

			return shadowMaps[0].get();
		}*/

	}
}
