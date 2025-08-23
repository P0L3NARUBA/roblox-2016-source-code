#pragma once

#include "TextureRef.h"
#include "util/G3DCore.h"

namespace RBX {
	class ContentId;
}

namespace G3D {
	class LightingParameters;
}

namespace RBX {
	namespace Graphics {

		class VisualEngine;
		class DeviceContext;
		class RenderCamera;
		class GeometryBatch;
		class VertexLayout;
		class VertexBuffer;

		class Sky {
		public:
			Sky(VisualEngine* visualEngine);
			~Sky();

			void update(const G3D::LightingParameters& lighting, uint32_t starCount, bool drawCelestialBodies, bool useHDRI);

			void render(DeviceContext* context, const RenderCamera& camera, const std::shared_ptr<Texture>& texture, bool drawStars = true);

			void RenderSkyboxEnvMapCube(DeviceContext* context, uint32_t face, uint32_t targetSize);

			void PrepareSkyboxEnvMapEqui(DeviceContext* context);
			void RenderSkyboxEnvMapBox(DeviceContext* context);

			void setSkyBoxDefault();
			void setSkyBox(const ContentId& rt, const ContentId& lf, const ContentId& bk, const ContentId& ft, const ContentId& up, const ContentId& dn, const ContentId& hdri);

			bool getUseHDRI() const { return useSkyboxHDRI; }

		private:
			/*struct Star
			{
				Vector3 position;
				float intensity;
			};

			struct StarData
			{
				std::vector<Star> stars;
				scoped_ptr<GeometryBatch> batch;
				shared_ptr<VertexBuffer> buffer;

				void resize(Sky* sky, unsigned int count, bool dynamic);
				void reset();
			};*/

			enum Brightness {
				Brightness_Bright,
				Brightness_Dimmer,
				Brightness_Dim,
				Brightness_None
			};

			VisualEngine* visualEngine;

			std::shared_ptr<VertexLayout> layout;

			std::unique_ptr<GeometryBatch> cube;

			std::array<std::shared_ptr<Texture>, 6u> skybox;
			std::shared_ptr<Texture> skyboxHDRI;

			Color3 skyColor;
			Color3 skyColor2;

			bool drawSunMoon;
			bool useSkyboxHDRI;

			Vector3 sunPosition;
			Color4 sunColor;
			std::shared_ptr<Texture> sun;

			Vector3 moonPosition;
			Color4 moonColor;
			std::shared_ptr<Texture> moon;

			/*Brightness starLight;
			StarData starsNormal;
			StarData starsTwinkle;
			int starTwinkleCounter;
			bool readyState;*/

			void loadSkyBoxDefault();
			void loadSkyBox(const ContentId& rt, const ContentId& lf, const ContentId& bk, const ContentId& ft, const ContentId& up, const ContentId& dn, const ContentId& hdri);

			/*void createStarField(int starCount);
			void updateStarsNormal();
			void updateStarsTwinkle();*/
		};

	}
}
