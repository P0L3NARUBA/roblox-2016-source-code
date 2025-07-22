#pragma once

#include "TextureRef.h"

#include "util/G3DCore.h"

namespace RBX
{
	class ContentId;
}

namespace G3D
{
	class LightingParameters;
}

namespace RBX
{
	namespace Graphics
	{

		class VisualEngine;
		class DeviceContext;
		class RenderCamera;
		class GeometryBatch;
		class VertexLayout;
		class VertexBuffer;

		class Sky
		{
		public:
			Sky(VisualEngine* visualEngine);
			~Sky();

			void update(const G3D::LightingParameters& lighting, int starCount, bool drawCelestialBodies);

			void prerender(); // call as early as possible, before the envmap update
			void render(DeviceContext* context, const RenderCamera& camera, Texture* texture, bool drawStars = true);

			void RenderSkyboxEnvMap(DeviceContext* context, unsigned int face, unsigned int targetSize);

			void setSkyBoxDefault();
			void setSkyBox(const ContentId& rt, const ContentId& lf, const ContentId& bk, const ContentId& ft, const ContentId& up, const ContentId& dn);
			bool isReady() const { return readyState; }

		private:
			struct Star
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
			};

			enum Brightness
			{
				Brightness_Bright,
				Brightness_Dimmer,
				Brightness_Dim,
				Brightness_None
			};

			VisualEngine* visualEngine;

			shared_ptr<VertexLayout> layout;

			scoped_ptr<GeometryBatch> cube;

			TextureRef skyBox[6];
			TextureRef skyBoxLoading[6];

			Color3 skyColor;
			Color3 skyColor2;

			bool drawSunMoon;
			Vector3 sunPosition;
			Color4 sunColor;
			TextureRef sun;

			Vector3 moonPosition;
			Color4 moonColor;
			TextureRef moon;

			Brightness starLight;
			StarData starsNormal;
			StarData starsTwinkle;
			int starTwinkleCounter;
			bool readyState;

			void loadSkyBoxDefault(TextureRef* textures);
			void loadSkyBox(TextureRef* textures, const ContentId& rt, const ContentId& lf, const ContentId& bk, const ContentId& ft, const ContentId& up, const ContentId& dn);

			void createStarField(int starCount);
			void updateStarsNormal();
			void updateStarsTwinkle();
		};

	}
}
