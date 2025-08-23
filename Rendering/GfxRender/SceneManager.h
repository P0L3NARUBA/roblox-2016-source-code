#pragma once

#include "GlobalShaderData.h"
#include "GfxCore/Resource.h"
#include "rbx/Boost.hpp"

#include "v8datamodel/Lighting.h"

#include "TextureRef.h"

using std::unique_ptr;

namespace RBX {
	namespace Graphics {

		class Sky;
		class SpatialHashedScene;
		class IndexBuffer;
		class RenderCamera;
		class RenderQueue;
		class VisualEngine;
		class Material;
		class DeviceContext;
		class CullableSceneNode;
		class GeometryBatch;
		class SSAO;
		class MSAA;
		class Bloom;
		class Blur;
		class ImageProcess;
		class Framebuffer;
		class EnvMap;
		class BlendState;
		class ShaderProgram;
		class ShadowMap;

		class SceneManager : Resource {
		public:

			struct PostProcessSettings {
				PostProcessSettings()
					: brightness(1.0f)
					, contrast(1.0f)
					, grayscaleLevel(1.0f)
					, tintColor(Color3::white())
					, tonemapper(ACES)

					, blurIntensity(0.0f)

					, bloomIntensity(0.0f)
					, bloomSize(0u)
				{
				}

				float brightness;
				float contrast;
				float grayscaleLevel;
				Color3 tintColor;
				TonemapperMode tonemapper;

				float blurIntensity;

				float bloomIntensity;
				uint32_t bloomSize;
			};

			SceneManager(VisualEngine* visualEngine);
			~SceneManager();

			SpatialHashedScene* getSpatialHashedScene() { return spatialHashedScene.get(); }

			Sky* getSky() { return sky.get(); }
			EnvMap* getEnvMap() { return envMap.get(); }
			SSAO* getSSAO() { return ssao.get(); }

			const GlobalShaderData& readGlobalShaderData() const { return globalShaderData; }
			GlobalShaderData& writeGlobalShaderData() { return globalShaderData; }
			const GlobalProcessingData& readGlobalProcessingData() const { return globalProcessingData; }
			GlobalProcessingData& writeGlobalProcessingData() { return globalProcessingData; }
			const GlobalLightList& readGlobalLightList() const { return globalLightList; }
			GlobalLightList& writeGlobalLightList() { return globalLightList; }

			// set the center of the culling sphere used by the framerate manager.
			const Vector3& getPointOfInterest() const { return pointOfInterest; }
			void setPointOfInterest(const Vector3& poi);

			void processSqPartDistance(float distance) {
				sqMinPartDistance = std::min(sqMinPartDistance, distance);
			}

			float getMinumumSqPartDistance() { return sqMinPartDistance; }
			const Vector3& getMinumumSqDistanceCenter() { return sqMinDistanceCenter; }

			void computeMinimumSqDistance(const RenderCamera& camera);

			void renderScene(DeviceContext* context, Framebuffer* mainFramebuffer, const RenderCamera& camera, uint32_t viewWidth, uint32_t viewHeight);

			void renderBegin(DeviceContext* context, const RenderCamera& camera, uint32_t viewWidth, uint32_t viewHeight);
			void renderView(DeviceContext* context, Framebuffer* mainFramebuffer, const RenderCamera& camera, uint32_t viewWidth, uint32_t viewHeight);
			void renderEnd(DeviceContext* context);

			void setSkyEnabled(bool value);
			void setClearColor(const Color4& value);
			void trackLightingTimeOfDay(double sec) { curTimeOfDay = sec; }

			void setFog(const Color3& color, float density, float sunInfluence, bool usesSkybox, bool affectsSkybox);
			void setLighting(const Color3& ambient, const Color3& outdoorAmbient, const Vector3& keyDirection, const Color3& keyColor);
			void setPostProcess(float brightness, float contrast, float grayscaleLevel, const Color3& tintColor, TonemapperMode tonemapper, float blurIntensity, float bloomIntensity, uint32_t bloomSize);
			void setProcessing(float exposure, float gamma);

			const GeometryBatch& getFullscreenTriangle() const { return *fullscreenTriangle; }

			void onDeviceLost();

			struct RenderTarget {
				std::shared_ptr<Texture> color;
				std::shared_ptr<Texture> depth;
				std::shared_ptr<Framebuffer> framebuffer;
			};

			const std::shared_ptr<Texture>& getShadowMapAtlas() const { return shadowMapAtlas; }
			const std::shared_ptr<Texture>& getShadowMapArray() const { return shadowMapArray; }

			const std::shared_ptr<Texture>& getAmbientOcclusion() const { return ambientOcclusion; }

		private:
			VisualEngine* visualEngine;

			Vector3 pointOfInterest;

			float   sqMinPartDistance;
			Vector3 sqMinDistanceCenter;

			unique_ptr<SpatialHashedScene> spatialHashedScene;

			std::vector<CullableSceneNode*> renderNodes;

			unique_ptr<RenderQueue> renderQueue;
			unique_ptr<RenderQueue> shadowRenderQueue;

			bool skyEnabled;

			Color4 clearColor;

			GlobalShaderData     globalShaderData;
			GlobalProcessingData globalProcessingData;
			GlobalLightList      globalLightList;

			unique_ptr<GeometryBatch> fullscreenTriangle;

			unique_ptr<Sky> sky;
			
			unique_ptr<RenderTarget> main;
			unique_ptr<RenderTarget> msaa;

			/*scoped_ptr<GBuffer> gbuffer;
			TextureRef gbufferColor;
			TextureRef gbufferDepth;*/

			unique_ptr<SSAO>         ssao;
			unique_ptr<Bloom>        bloom;
			unique_ptr<Blur>         blur;
			unique_ptr<EnvMap>       envMap;
			unique_ptr<ImageProcess> imageProcess;

			std::shared_ptr<Texture> shadowMapAtlas;
			std::shared_ptr<Texture> shadowMapArray;

			std::shared_ptr<Texture> ambientOcclusion;

			/*scoped_ptr<ShadowMap> shadowMaps[3];
			float shadowMapTexelSize;
			TextureRef shadowMask;
			TextureRef shadowMapTexture;*/

			PostProcessSettings postProcessSettings;

			double curTimeOfDay;
			//bool   gbufferError;
			bool   msaaError;

			/*struct ShadowValues {
				float intensity;
			};

			ShadowValues unsetAndGetShadowValues(DeviceContext* context);
			void restoreShadows(DeviceContext* context, const ShadowValues& shadowValues);*/

			void updateMain(uint32_t width, uint32_t height);
			void updateMSAA(uint32_t width, uint32_t height);

			/*void updateGBuffer(unsigned width, unsigned height);
			void resolveGBuffer(DeviceContext* context, Texture* texture);

			void renderShadowMap(DeviceContext* context, ShadowMap* shadowMap);
			RenderCamera getShadowCamera(const Vector3& center, int shadowMapSize, float shadowRadius, float shadowDepthRadius);
			void blurShadowMap(DeviceContext* context, ShadowMap* shadowMap);

			ShadowMap* pickShadowMap(int qualityLevel) const;*/
		};

	}
}
