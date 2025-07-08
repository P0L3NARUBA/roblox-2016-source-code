#pragma once

#include "GlobalShaderData.h"
#include "GfxCore/Resource.h"
#include "rbx/Boost.hpp"

#include "v8datamodel/Lighting.h"

#include "TextureRef.h"

namespace RBX
{
	namespace Graphics
	{

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

		class SceneManager : Resource
		{
		public:

			struct PostProcessSettings
			{
				PostProcessSettings()
					: brightness(0.0f)
					, contrast(0.0f)
					, grayscaleLevel(0.0f)
					, blurIntensity(0.0f)
					, bloomIntensity(0.0f)
					, bloomSize(0)
					, tintColor(Color3::white()) {
				}

				float brightness;
				float contrast;
				float grayscaleLevel;
				float blurIntensity;
				float bloomIntensity;
				int bloomSize;
				Color3 tintColor;
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

			void processSqPartDistance(float distance)
			{
				sqMinPartDistance = std::min(sqMinPartDistance, distance);
			}

			float getMinumumSqPartDistance() { return sqMinPartDistance; }
			const Vector3& getMinumumSqDistanceCenter() { return sqMinDistanceCenter; }

			void computeMinimumSqDistance(const RenderCamera& camera);

			void renderScene(DeviceContext* context, Framebuffer* mainFramebuffer, const RenderCamera& camera, unsigned int viewWidth, unsigned int viewHeight);

			void renderBegin(DeviceContext* context, const RenderCamera& camera, unsigned int viewWidth, unsigned int viewHeight);
			void renderView(DeviceContext* context, Framebuffer* mainFramebuffer, const RenderCamera& camera, unsigned int viewWidth, unsigned int viewHeight);
			void renderEnd(DeviceContext* context);

			void setSkyEnabled(bool value);
			void setClearColor(const Color4& value);
			void setFog(const Color3& color, const Vector4& fogParams);
			void setLighting(const Color3& ambient, const Color3& outdoorAmbient, const Vector3& keyDirection, const Color3& keyColor);
			void trackLightingTimeOfDay(double sec) { curTimeOfDay = sec; }
			void setPostProcess(float brightness, float contrast, float grayscaleLevel, float blurIntensity, const Color3& tintColor, float bloomIntensity, int bloomSize);

			const GeometryBatch& getFullscreenTriangle() const { return *fullscreenTriangle; }

			void onDeviceLost();

			/*struct GBuffer
			{
				shared_ptr<Texture> mainColor;
				shared_ptr<Framebuffer> mainFB;

				shared_ptr<Texture> gbufferColor;
				shared_ptr<Framebuffer> gbufferColorFB;

				shared_ptr<Texture> gbufferDepth;
				shared_ptr<Framebuffer> gbufferDepthFB;

				shared_ptr<Framebuffer> gbufferFB;
			};*/

			struct Main
			{
				shared_ptr<Texture> mainColor;
				shared_ptr<Texture> mainDepth;
				shared_ptr<Framebuffer> mainFB;
			};

			/*GBuffer* getGBuffer() const { return gbuffer.get(); }
			const TextureRef& getGBufferColor() const { return gbufferColor; }
			const TextureRef& getGBufferDepth() const { return gbufferDepth; }

			const TextureRef& getShadowMap() const { return shadowMapTexture; }*/

		private:
			VisualEngine* visualEngine;

			Vector3 pointOfInterest;

			float sqMinPartDistance;
			Vector3 sqMinDistanceCenter;

			scoped_ptr<SpatialHashedScene> spatialHashedScene;

			std::vector<CullableSceneNode*> renderNodes;
			scoped_ptr<RenderQueue> renderQueue;
			scoped_ptr<RenderQueue> shadowRenderQueue;

			bool skyEnabled;

			Color4 clearColor;

			GlobalShaderData globalShaderData;
			GlobalProcessingData globalProcessingData;
			GlobalLightList globalLightList;

			scoped_ptr<GeometryBatch> fullscreenTriangle;

			scoped_ptr<Sky> sky;

			scoped_ptr<Main> main;
			TextureRef mainColor;
			TextureRef mainDepth;

			/*scoped_ptr<GBuffer> gbuffer;
			TextureRef gbufferColor;
			TextureRef gbufferDepth;*/

			scoped_ptr<SSAO> ssao;
			scoped_ptr<MSAA> msaa;
			scoped_ptr<Bloom> bloom;
			scoped_ptr<Blur> blur;
			scoped_ptr<EnvMap> envMap;
			scoped_ptr<ImageProcess> imageProcess;

			/*scoped_ptr<ShadowMap> shadowMaps[3];
			float shadowMapTexelSize;
			TextureRef shadowMask;
			TextureRef shadowMapTexture;*/

			PostProcessSettings postProcessSettings;

			double curTimeOfDay;
			bool   gbufferError; // when set to true, gbuffer is permanently disabled
			bool   msaaError;

			struct ShadowValues
			{
				float intensity;
			};

			/*ShadowValues unsetAndGetShadowValues(DeviceContext* context);
			void restoreShadows(DeviceContext* context, const ShadowValues& shadowValues);*/

			void updateMain(unsigned width, unsigned height);

			void updateMSAA(unsigned width, unsigned height);

			/*void updateGBuffer(unsigned width, unsigned height);
			void resolveGBuffer(DeviceContext* context, Texture* texture);

			void renderShadowMap(DeviceContext* context, ShadowMap* shadowMap);
			RenderCamera getShadowCamera(const Vector3& center, int shadowMapSize, float shadowRadius, float shadowDepthRadius);
			void blurShadowMap(DeviceContext* context, ShadowMap* shadowMap);

			ShadowMap* pickShadowMap(int qualityLevel) const;*/
		};

	}
}
