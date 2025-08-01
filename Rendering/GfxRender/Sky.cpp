#include "stdafx.h"
#include "Sky.h"

#include "VisualEngine.h"
#include "TextureManager.h"
#include "ShaderManager.h"
#include "ScreenSpaceEffect.h"
#include "SceneManager.h"
#include "Util.h"
#include "EnvMap.h"
#include "Vertex.h"

#include "GfxCore/Geometry.h"
#include "GfxCore/Device.h"
#include "GfxCore/States.h"

#include "v8datamodel/Sky.h"

#include "g3d/LightingParameters.h"
#include "GfxCore/pix.h"

#include "rbx/Profiler.h"

FASTFLAG(DebugRenderDownloadAssets)
FASTFLAGVARIABLE(RenderMoonBillboard, true)

namespace RBX {
	namespace Graphics {

		static const float kSunSize = 25.0f;
		static const float kMoonSize = 25.0f;
		static const float kSkySize = 2700.0f;
		static const float kStarDistance = 2500.0f;
		static const float kStarTwinkleRatio = 0.2f;
		static const uint32_t kStarTwinkleRate = 40u;

		static TextureRef::Status getCombinedStatus(const TextureRef(&textures)[6]) {
			for (uint32_t i = 0u; i < 6u; ++i) {
				TextureRef::Status status = textures[i].getStatus();

				if (status != TextureRef::Status_Loaded)
					return status;
			}

			return TextureRef::Status_Loaded;
		}

		static Matrix4 getBillboardTransform(const RenderCamera& camera, const Vector3& position, float size) {
			if (FFlag::RenderMoonBillboard) {
				Matrix3 rotation = Math::getWellFormedRotForZVector(normalize(position - camera.getPosition()));

				return Matrix4(rotation * Matrix3::fromDiagonal(Vector3(size)), position);
			}
			else {
				Matrix4 viewInverse = camera.getViewMatrix().inverse();

				return Matrix4(viewInverse.upper3x3() * Matrix3::fromDiagonal(Vector3(size)), position);
			}
		}

		static shared_ptr<Geometry> createCube(Device* device) {
			std::vector<VertexLayout::Element> elements;
			elements.push_back(VertexLayout::Element(0u, 0u, VertexLayout::Format_Float3, VertexLayout::Input_Vertex, VertexLayout::Semantic_Position));

			shared_ptr<VertexLayout> layout = device->createVertexLayout(elements);

			Vector3 vertices[] = {
				Vector3(-1.0f,  1.0f, -1.0f), /* 3 - -     */
				Vector3(-1.0f, -1.0f, -1.0f), /* - - -  -X */
				Vector3(-1.0f, -1.0f,  1.0f), /* 2 - 1     */

				Vector3(-1.0f, -1.0f,  1.0f), /* 1 - 2     */
				Vector3(-1.0f,  1.0f,  1.0f), /* - - -  -X */
				Vector3(-1.0f,  1.0f, -1.0f), /* - - 3     */

				Vector3( 1.0f,  1.0f, -1.0f), /* 3 - -     */
				Vector3( 1.0f, -1.0f, -1.0f), /* - - -   X */
				Vector3( 1.0f, -1.0f,  1.0f), /* 2 - 1     */

				Vector3( 1.0f, -1.0f,  1.0f), /* 1 - 2     */
				Vector3( 1.0f,  1.0f,  1.0f), /* - - -   X */
				Vector3( 1.0f,  1.0f, -1.0f), /* - - 3     */

				Vector3( 1.0f, -1.0f, -1.0f), /* 3 - -     */
				Vector3(-1.0f, -1.0f, -1.0f), /* - - -  -Y */
				Vector3(-1.0f, -1.0f,  1.0f), /* 2 - 1     */

				Vector3(-1.0f, -1.0f,  1.0f), /* 1 - 2     */
				Vector3( 1.0f, -1.0f,  1.0f), /* - - -  -Y */
				Vector3( 1.0f, -1.0f, -1.0f), /* - - 3     */

				Vector3( 1.0f,  1.0f, -1.0f), /* 3 - -     */
				Vector3(-1.0f,  1.0f, -1.0f), /* - - -   Y */
				Vector3(-1.0f,  1.0f,  1.0f), /* 2 - 1     */

				Vector3(-1.0f,  1.0f,  1.0f), /* 1 - 2     */
				Vector3( 1.0f,  1.0f,  1.0f), /* - - -   Y */
				Vector3( 1.0f,  1.0f, -1.0f), /* - - 3     */

				Vector3( 1.0f, -1.0f, -1.0f), /* 3 - -     */
				Vector3(-1.0f, -1.0f, -1.0f), /* - - -  -Z */
				Vector3(-1.0f,  1.0f, -1.0f), /* 2 - 1     */

				Vector3(-1.0f,  1.0f, -1.0f), /* 1 - 2     */
				Vector3( 1.0f,  1.0f, -1.0f), /* - - -  -Z */
				Vector3( 1.0f, -1.0f, -1.0f), /* - - 3     */

				Vector3( 1.0f, -1.0f,  1.0f), /* 3 - -     */
				Vector3(-1.0f, -1.0f,  1.0f), /* - - -   Z */
				Vector3(-1.0f,  1.0f,  1.0f), /* 2 - 1     */

				Vector3(-1.0f,  1.0f,  1.0f), /* 1 - 2     */
				Vector3( 1.0f,  1.0f,  1.0f), /* - - -   Z */
				Vector3( 1.0f, -1.0f,  1.0f), /* - - 3     */
			};

			shared_ptr<VertexBuffer> vb = device->createVertexBuffer(sizeof(Vector3), ARRAYSIZE(vertices), GeometryBuffer::Usage_Static);

			vb->upload(0u, vertices, sizeof(vertices));

			return device->createGeometry(layout, vb, shared_ptr<IndexBuffer>());
		}

		/*void Sky::StarData::resize(Sky* sky, unsigned int count, bool dynamic)
		{
			if (count == 0)
				reset();
			else
			{
				stars.resize(count);

				buffer = sky->visualEngine->getDevice()->createVertexBuffer(sizeof(SkyVertex), count, dynamic ? GeometryBuffer::Usage_Dynamic : GeometryBuffer::Usage_Static);

				batch.reset(new GeometryBatch(sky->visualEngine->getDevice()->createGeometry(sky->layout, buffer, shared_ptr<IndexBuffer>()), Geometry::Primitive_Points, count, count));
			}
		}

		void Sky::StarData::reset()
		{
			stars.clear();
			batch.reset();
			buffer.reset();
		}*/

		Sky::Sky(VisualEngine* visualEngine)
			: visualEngine(visualEngine)
			/*, starLight(Brightness_None)
			, starTwinkleCounter(0)
			, readyState(false)*/
		{
			cube.reset(new GeometryBatch(createCube(visualEngine->getDevice()), Geometry::Primitive_Triangles, 36u, 36u));

			// preload default skybox so that we can show it even while fetching custom skies over HTTP
			//if (!FFlag::DebugRenderDownloadAssets)
			loadSkyBoxDefault();

			// preload sun/mon
			sun = visualEngine->getTextureManager()->load(ContentId("rbxasset://sky/sun.dds"), TextureManager::Fallback_BlackTransparent);
			moon = visualEngine->getTextureManager()->load(ContentId("rbxasset://sky/moon.dds"), TextureManager::Fallback_BlackTransparent);
		}

		Sky::~Sky()
		{
		}

		void Sky::update(const G3D::LightingParameters& lighting, uint32_t starCount, bool drawCelestialBodies, bool useHDRI) {
			skyColor = lighting.skyAmbient;
			skyColor2 = lighting.skyAmbient2;

			drawSunMoon = drawCelestialBodies;
			useSkyboxHDRI = useHDRI;

			sunPosition = (lighting.physicallyCorrect ? lighting.trueSunPosition : lighting.sunPosition);
			sunColor = Color3(lighting.emissiveScale * 0.8f * G3D::clamp((sunPosition.y + 0.1f) * 10.0f, 0.f, 1.f));

			moonPosition = (lighting.physicallyCorrect ? lighting.trueMoonPosition : lighting.moonPosition);
			moonColor = Color3(G3D::min((1.0f - lighting.skyAmbient[1]) + 0.1f, 1.0f));

			/*if (!drawCelestialBodies || starCount == 0)
			{
				starsNormal.reset();
				starsTwinkle.reset();
			}
			else
			{
				Brightness oldStarLight = starLight;

				if (lighting.moonPosition.y <= -0.3)
					starLight = Brightness_None;
				else if (lighting.moonPosition.y <= -0.1)
					starLight = Brightness_Dim;
				else if (lighting.moonPosition.y <= 0.2)
					starLight = Brightness_Dimmer;
				else
					starLight = Brightness_Bright;

				if (starsNormal.stars.size() + starsTwinkle.stars.size() != starCount)
				{
					createStarField(starCount);

					updateStarsNormal();
					updateStarsTwinkle();
				}
				else if (oldStarLight != starLight)
				{
					updateStarsNormal();
					updateStarsTwinkle();
				}
			}*/
		}

		/*void Sky::prerender()
		{
			TextureRef::Status loadingStatus = getCombinedStatus(skyBoxLoading);

			if (loadingStatus == TextureRef::Status_Loaded)
			{
				std::copy(skyBoxLoading, skyBoxLoading + 6, skyBox);
				readyState = true;

				if (EnvMap* envMap = visualEngine->getSceneManager()->getEnvMap())
				{
					//envMap->markDirty(false);
				}
			}

			if (loadingStatus != TextureRef::Status_Waiting)
				std::fill(skyBoxLoading, skyBoxLoading + 6, TextureRef());

			// Update star twinkle
			starTwinkleCounter++;
		}*/

		void Sky::RenderSkyboxEnvMapCube(DeviceContext* context, uint32_t face, uint32_t size) {
			if (ShaderProgram* program = ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "PassThroughVS", "SkyCubeFaceFS", BlendState::Mode_None, size, size)) {
				context->bindTexture(0u, skybox[face].getTexture().get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

				ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
			}
		}

		void Sky::PrepareSkyboxEnvMapEqui(DeviceContext* context) {
			shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("SkyVS", "SkyEquiFaceFS");

			if (program) {
				context->bindProgram(program.get());

				context->setRasterizerState(RasterizerState::Cull_None);
				context->setDepthState(DepthState(DepthState::Function_Always, false));
				context->setBlendState(BlendState::Mode_None);

				context->bindTexture(0u, skyboxHDRI.getTexture().get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));
			}
		}

		void Sky::RenderSkyboxEnvMapBox(DeviceContext* context) {
			context->draw(*cube);
		}

		void Sky::render(DeviceContext* context, const RenderCamera& camera, Texture* texture, bool drawStars) {
			RBXPROFILER_SCOPE("Render", "Sky");
			RBXPROFILER_SCOPE("GPU", "Sky");
			PIX_SCOPE(context, "Sky");

			// Update load-in-progress
			/*if (starTwinkleCounter >= kStarTwinkleRate) {
				starTwinkleCounter = 0;

				if (starLight != Brightness_None && !starsTwinkle.stars.empty())
					updateStarsTwinkle();
			}*/

			shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("SkyVS", "SkyFS");

			if (program) {
				context->bindProgram(program.get());

				context->setRasterizerState(RasterizerState::Cull_None);
				context->setDepthState(DepthState(DepthState::Function_GreaterEqual, false));
				context->setBlendState(BlendState::Mode_None);

				context->bindTexture(0u, texture, SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Wrap));

				context->draw(*cube);
			}

			// Render stars
			/*if (drawStars && starLight != Brightness_None && (starsNormal.batch || starsTwinkle.batch))
			{
				PIX_SCOPE(context, "Stars");
				context->setBlendState(BlendState::Mode_AlphaBlend);

				float colorData[] = { 1, 1, 1, 1 };
				context->setConstant(colorHandle, colorData, 1);
				context->setConstant(color2Handle, colorData, 1);

				context->bindTexture(0, visualEngine->getTextureManager()->getFallbackTexture(TextureManager::Fallback_White).get(), SamplerState::Filter_Linear);

				Matrix4 transform(Matrix3::identity(), camera.getPosition());

				context->setWorldTransforms4x3(transform[0], 1);

				if (starsNormal.batch)
					context->draw(*starsNormal.batch);

				if (starLight != Brightness_Dim && starsTwinkle.batch)
					context->draw(*starsTwinkle.batch);
			}*/

			// Render sun/moon
			/*if (drawSunMoon)
			{
				PIX_SCOPE(context, "Sun");
				context->setBlendState(BlendState::Mode_Additive);

				Matrix4 sunTransform = getBillboardTransform(camera, camera.getPosition() + sunPosition * kSkySize, kSunSize);

				//context->setConstant(colorHandle, &sunColor.r, 1);
				//context->setConstant(color2Handle, &sunColor.r, 1);

				context->bindTexture(0, sun.getTexture().get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

				context->setWorldTransforms4x3(sunTransform[0], 1);
				context->draw(*quad);
				//context->draw(*quad);
				//context->draw(*quad);

				PIX_MARKER(context, "Moon");
				context->setBlendState(BlendState::Mode_AlphaBlend);

				Matrix4 moonTransform = getBillboardTransform(camera, camera.getPosition() + moonPosition * kSkySize, kMoonSize);

				//context->setConstant(colorHandle, &moonColor.r, 1);

				context->bindTexture(0, moon.getTexture().get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

				context->setWorldTransforms4x3(moonTransform[0], 1);
				context->draw(*quad);
			}*/
		}

		void Sky::setSkyBox(const ContentId& rt, const ContentId& lf, const ContentId& bk, const ContentId& ft, const ContentId& up, const ContentId& dn, const ContentId& hdri) {
			loadSkyBox(rt, lf, bk, ft, up, dn, hdri);
		}

		void Sky::setSkyBoxDefault() {
			loadSkyBoxDefault();
		}

		void Sky::loadSkyBoxDefault() {
			RBX::Sky dummy;

			loadSkyBox(dummy.getSkyboxRt(), dummy.getSkyboxLf(), dummy.getSkyboxBk(), dummy.getSkyboxFt(), dummy.getSkyboxUp(), dummy.getSkyboxDn(), dummy.getSkyboxHDRI());
		}

		void Sky::loadSkyBox(const ContentId& rt, const ContentId& lf, const ContentId& bk, const ContentId& ft, const ContentId& up, const ContentId& dn, const ContentId& hdri) {
			TextureManager* tm = visualEngine->getTextureManager();

			skybox[NORM_X_POS] = tm->load(rt, TextureManager::Fallback_Black, "Sky.Back");
			skybox[NORM_X_NEG] = tm->load(lf, TextureManager::Fallback_Black, "Sky.Front");
			skybox[NORM_Y_POS] = tm->load(up, TextureManager::Fallback_Black, "Sky.Up");
			skybox[NORM_Y_NEG] = tm->load(dn, TextureManager::Fallback_Black, "Sky.Down");
			skybox[NORM_Z_POS] = tm->load(bk, TextureManager::Fallback_Black, "Sky.Left");
			skybox[NORM_Z_NEG] = tm->load(ft, TextureManager::Fallback_Black, "Sky.Right");
			skyboxHDRI = tm->load(hdri, TextureManager::Fallback_Black, "Sky.Right");
		}

		/*void Sky::createStarField(int starCount)
		{
			int twinkle = static_cast<int>(starCount * kStarTwinkleRatio);
			int normal = starCount - twinkle;

			// create random positions and intensities for regular stars
			starsNormal.resize(this, normal, false);

			for (size_t i = 0; i < starsNormal.stars.size(); ++i)
			{
				Star& s = starsNormal.stars[i];

				Vector3 r = Vector3::random();

				s.position = r * kStarDistance;
				s.intensity = G3D::uniformRandom(0.3f, 0.8f);

				float tpos = fabs(r.y);

				if (tpos < 0.15f)
					s.intensity *= 0.6f;
				else if (tpos < 0.3f)
					s.intensity *= 0.75f;
				else if (tpos < 0.6f)
					s.intensity *= 0.9f;

				s.intensity *= s.intensity;
			}

			// create random positions and intensities for twinkling stars
			starsTwinkle.resize(this, twinkle, true);

			for (size_t i = 0; i < starsTwinkle.stars.size(); ++i)
			{
				Star& s = starsTwinkle.stars[i];

				Vector3 r = Vector3::random();

				// only top hemisphere has twinkling stars
				r.y = fabsf(r.y);

				s.position = r * kStarDistance;
				s.intensity = G3D::uniformRandom(0.2f, 0.7f);

				float tpos = fabs(r.y);

				if (tpos < 0.3f)
					s.intensity *= 0.7f;
				else if (tpos < 0.6f)
					s.intensity *= 0.9f;

				s.intensity *= s.intensity;
			}
		}

		void Sky::updateStarsNormal()
		{
			bool colorOrderBGR = visualEngine->getDevice()->getCaps().colorOrderBGR;

			const StarData& data = starsNormal;

			if (data.stars.empty())
				return;

			SkyVertex* vertices = static_cast<SkyVertex*>(data.buffer->lock());

			for (unsigned int i = 0; i < data.stars.size(); ++i)
			{
				vertices[i].Position = data.stars[i].position;

				float intensity = data.stars[i].intensity;

				if (starLight == Brightness_Bright)
				{
					intensity *= 5 * 0.3f;
				}
				else if (starLight == Brightness_Dimmer)
				{
					intensity *= 2.5f * 0.15f;
				}
				else
				{
					intensity *= 0.6f;
				}

				float tnum = G3D::uniformRandom(0.0, 1.0);

				if (tnum < 0.15)
					vertices[i].color = packColor(Color4(1.0, 0.80f, 0.70f, intensity), colorOrderBGR);	// yellow (15%)
				else if (tnum < 0.55)
					vertices[i].color = packColor(Color4(1.0, 1.0, 1.0, intensity), colorOrderBGR);   // white (40%)
				else if (tnum < 0.80)
					vertices[i].color = packColor(Color4(0.70f, 0.80f, 1.0, intensity), colorOrderBGR);	// blue (25%)
				else
					vertices[i].color = packColor(Color4(0.75f, 0.50f, 1.0, intensity), colorOrderBGR);	// purple (20%)

				vertices[i].texcoord = Vector2(0, 0);
			}

			data.buffer->unlock();
		}

		void Sky::updateStarsTwinkle()
		{
			bool colorOrderBGR = visualEngine->getDevice()->getCaps().colorOrderBGR;

			const StarData& data = starsTwinkle;

			if (data.stars.empty())
				return;

			SkyVertex* vertices = static_cast<SkyVertex*>(data.buffer->lock(GeometryBuffer::Lock_Discard));

			for (unsigned int i = 0; i < data.stars.size(); ++i)
			{
				vertices[i].Position = data.stars[i].position;

				float randomBrightness = G3D::uniformRandom(0.3f, 1.7f);
				float brightfactor = (starLight == Brightness_Dimmer) ? 0.5f : 1.f;
				float intensity = data.stars[i].intensity * randomBrightness * brightfactor + 0.2f;

				float tnum = G3D::uniformRandom(0.0, 1.0);

				if (tnum < 0.15)
					vertices[i].color = packColor(Color4(1.0, 0.80f, 0.70f, intensity), colorOrderBGR);	// yellow (15%)
				else if (tnum < 0.55)
					vertices[i].color = packColor(Color4(1.0, 1.0, 1.0, intensity), colorOrderBGR);   // white (40%)
				else if (tnum < 0.80)
					vertices[i].color = packColor(Color4(0.70f, 0.80f, 1.0, intensity), colorOrderBGR);	// blue (25%)
				else
					vertices[i].color = packColor(Color4(0.75f, 0.50f, 1.0, intensity), colorOrderBGR);	// purple (20%)

				vertices[i].texcoord = Vector2(0, 0);
			}

			data.buffer->unlock();
		}*/

	}
}
