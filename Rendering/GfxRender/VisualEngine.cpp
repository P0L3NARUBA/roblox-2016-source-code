#include "stdafx.h"
#include "VisualEngine.h"

#include "GfxBase/RenderCaps.h"
#include "GfxBase/RenderStats.h"
#include "GfxBase/FrameRateManager.h"

#include "GfxCore/Device.h"

#include "GlobalShaderData.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "TextureCompositor.h"
#include "Water.h"
#include "EmitterShared.h"
#include "MaterialGenerator.h"
#include "SceneManager.h"
#include "TypesetterBitmap.h"
#include "TypesetterDynamic.h"
#include "AdornRender.h"
#include "VertexStreamer.h"
#include "SceneUpdater.h"
#include "SmoothCluster.h"
#include "TextureAtlas.h"
#include "MeshInstancer.h"

#include "GfxBase/Typesetter.h"

#include "v8datamodel/DataModel.h"
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/MeshContentProvider.h"
#include "v8datamodel/SolidModelContentProvider.h"
#include "v8datamodel/Camera.h"
#include "v8datamodel/MegaCluster.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/Lighting.h"
#include "rbx/SystemUtil.h"

LOGGROUP(Graphics)

FASTFLAGVARIABLE(CancelPendingTextureLoads, true)
FASTFLAG(UseDynamicTypesetterUTF8)

FASTFLAG(CameraVR)

namespace RBX {
	namespace Graphics {

		VisualEngine::VisualEngine(Device* device, CRenderSettings* settings)
			: device(device)
			, viewWidth(0u)
			, viewHeight(0u)
			, settings(settings)
			, contentProvider(nullptr)
			, lighting(nullptr)
			, meshContentProvider(nullptr)
		{
			renderStats.reset(new RenderStats());
			renderCaps.reset(new RenderCaps("Unknown", SystemUtil::getVideoMemory()));

			FASTLOG1(FLog::Graphics, "Video memory size: %lld", SystemUtil::getVideoMemory());

			GlobalShaderData::define(device);
			GlobalProcessingData::define(device);
			GlobalMaterialData::define(device);
			GlobalLightList::define(device);

			InstancedModels::define(device);

			// load shaders
			shaderManager.reset(new ShaderManager(this));
			shaderManager->loadShaders(ContentProvider::assetFolder() + "../shaders", device->getShadingLanguage(), /* consoleOutput= */ false);

			// initialize texture manager
			textureManager.reset(new TextureManager(this));

			// load fonts
			if (FFlag::UseDynamicTypesetterUTF8) {
				glyphAtlas.reset(new TextureAtlas(this, 2048u, 2048u));

				for (Text::Font font = Text::FONT_LEGACY; font != Text::FONT_LAST; font = Text::Font(font + 1)) {
					static const char* kFontTTFPaths[] = {
						"fonts/arial.ttf",
						"fonts/arial.ttf",
						"fonts/arialbd.ttf",
						"fonts/SourceSansPro-Regular.ttf",
						"fonts/SourceSansPro-Bold.ttf",
						"fonts/SourceSansPro-Light.ttf",
						"fonts/SourceSansPro-It.ttf",
						"fonts/AccanthisADFStd-Regular.otf",
						"fonts/Guru-Regular.otf",
						"fonts/ComicNeue-Angular-Bold.ttf",
						"fonts/Inconsolata-Regular.ttf",
						"fonts/HWYGOTH.ttf",
						"fonts/zekton_rg.ttf",
						"fonts/PressStart2P-Regular.ttf",
						"fonts/Balthazar-Regular.ttf",
						"fonts/RomanAntique.otf",
						"fonts/SourceSansPro-Semibold.ttf",
						"fonts/GothamSSm-Book.ttf",
						"fonts/GothamSSm-Medium.ttf",
						"fonts/GothamSSm-Bold.ttf",
						"fonts/GothamSSm-Black.ttf"
					};

					float legacyHeightScale = (font == Text::FONT_LEGACY) ? 1.5f : 1.0f;
					typesetters[font].reset(new TypesetterDynamic(glyphAtlas.get(), textureManager.get(), ContentProvider::assetFolder() + kFontTTFPaths[font], legacyHeightScale, (unsigned)font, device->getCaps().retina));
				}
			}
			else {
				for (Text::Font font = Text::FONT_LEGACY; font != Text::FONT_LAST; font = Text::Font(font + 1)) {
					static const char* kFontPaths[Text::FONT_LAST] = {
						"fonts/Arial.font", "fonts/Arial.font", "fonts/ArialBold.font", "fonts/SourceSans.font", "fonts/SourceSansBold.font",
						"fonts/SourceSansLight.font", "fonts/SourceSansItalic.font",
					};

					static const char* kTexturePath = "rbxasset://fonts/fonts.dds";

					float legacyHeightScale = (font == Text::FONT_LEGACY) ? 1.5f : 1.0f;
					typesetters[font].reset(new TypesetterBitmap(getTextureManager(), ContentProvider::assetFolder() + kFontPaths[font], kTexturePath, legacyHeightScale, device->getCaps().retina));
				}
			}

			materialGenerator.reset(new MaterialGenerator(this));

			sceneManager.reset(new SceneManager(this));

			frameRateManager.reset(new FrameRateManager());

			vertexStreamer.reset(new VertexStreamer(this));

			textureCompositor.reset(new TextureCompositor(this));

			water.reset(new Water(this));

			emitterShared.reset(new EmitterShared);

			// set up caps
			bool gbufferSupported = false;
			/*shaderManager->getProgram("PassThroughVS", "SSAOFS").get() != NULL &&
			device->getCaps().maxDrawBuffers >= 2 &&
			(device->getFeatureLevel() == "D3D11" || SystemUtil::getVideoMemory() >= 128*1024*1024);*/

			renderCaps->setSupportsGBuffer(gbufferSupported);

			if (shared_ptr<ShaderProgram> program = shaderManager->getProgram("DefaultSkinnedVS", "DefaultFS")) {
				uint32_t boneCount = program->getMaxWorldTransforms();

				FASTLOG1(FLog::Graphics, "Supported bones for skinning: %d", boneCount);

				renderCaps->setSkinningBoneCount(boneCount);
			}
			else {
				FASTLOG(FLog::Graphics, "Supported bones for skinning: 0 (no shader support)");
			}

			// configure FRM after caps are valid
			frameRateManager->Configure(renderCaps.get(), settings);
		}

		const shared_ptr<Typesetter>& VisualEngine::getTypesetter(Text::Font font) {
			return typesetters[font];
		}

		VisualEngine::~VisualEngine() {
			bindWorkspace(shared_ptr<DataModel>());
		}

		void VisualEngine::bindWorkspace(const shared_ptr<DataModel>& dm) {
			if (dm) {
				contentProvider = ServiceProvider::create<ContentProvider>(dm.get());
				meshContentProvider = ServiceProvider::create<MeshContentProvider>(dm.get());
				lighting = ServiceProvider::create<Lighting>(dm.get());
				meshContentProvider->setCacheSize(settings->getMeshCacheSize());
				ServiceProvider::create<SolidModelContentProvider>(dm.get());

				//sceneUpdater.reset(new SceneUpdater(dm, this));
				meshInstancer.reset(new MeshInstancer(dm, this));

				adorn.reset(new AdornRender(this, dm.get()));
			}
			else {
				contentProvider = nullptr;
				meshContentProvider = nullptr;
				lighting = nullptr;

				/*if (sceneUpdater) {
					sceneUpdater->unbind();
					sceneUpdater.reset();
				}*/

				if (meshInstancer) {
					meshInstancer->unbind();
					meshInstancer.reset();
				}

				adorn.reset();

				if (FFlag::CancelPendingTextureLoads) {
					textureCompositor->cancelPendingRequests();
					textureManager->cancelPendingRequests();
				}
			}
		}

		void VisualEngine::setViewport(uint32_t width, uint32_t height)
		{
			viewWidth = width;
			viewHeight = height;
		}

		void VisualEngine::setCamera(const Camera& value, const G3D::Vector3& poi) {
			if (FFlag::CameraVR)
				camera.setViewCFrame(value.getRenderingCoordinateFrame());
			else
				camera.setViewCFrame(value.coordinateFrame(), value.getRoll());

			camera.setProjectionPerspective(value.getFieldOfView(), static_cast<float>(viewWidth) / viewHeight, -value.nearPlaneZ(), -value.farPlaneZ());

			sceneManager->setPointOfInterest(poi);

			if (sceneUpdater)
				sceneUpdater->setPointOfInterest(poi);

			// get frustum with the far clip plane to max block distance
			/*float updateFarPlaneZ;
			if (frameRateManager) {
				updateFarPlaneZ = (float) -std::max(1.0, frameRateManager->GetMaxNextViewCullDistance());
				updateFarPlaneZ = std::max(updateFarPlaneZ, value.farPlaneZ());
			} else {
				updateFarPlaneZ = value.farPlaneZ();
			}

			value.frustum(updateFarPlaneZ, updateFrustum);*/

			value.frustum(value.farPlaneZ(), updateFrustum);

			// update camera for culling
			cameraCull = camera;

			if (DeviceVR* vr = device->getVR()) {
				DeviceVR::State vrState = vr->getState();

				// Use max FOV for culling; ideally we also should account for IPD
				cameraCull.setProjectionPerspective(
					std::max(vrState.eyeFov[0][0], vrState.eyeFov[1][0]),
					std::max(vrState.eyeFov[0][1], vrState.eyeFov[1][1]),
					std::max(vrState.eyeFov[0][2], vrState.eyeFov[1][2]),
					std::max(vrState.eyeFov[0][3], vrState.eyeFov[1][3]),
					-value.nearPlaneZ(), -value.farPlaneZ());
			}

			// update camera for FRM culling
			cameraCullFrm = cameraCull;

			cameraCullFrm.changeProjectionPerspectiveZ(-value.nearPlaneZ(), -value.farPlaneZ()); //std::min(-value.farPlaneZ(), sqrtf(frameRateManager->GetRenderCullSqDistance())));
		}

		void VisualEngine::reloadShaders() {
			shaderManager->loadShaders(ContentProvider::assetFolder() + "../shaders", device->getShadingLanguage(), /* consoleOutput= */ true);
		}

		void VisualEngine::reloadQueuedAssets() {
			for (FilenameCountdown::iterator it = assetsToReload.begin(); it != assetsToReload.end(); ++it) {
				--it->second;

				if (it->second == 0u) {
					const std::string& filePath = it->first;
					immediateAssetReload(filePath);
					assetsToReload.erase(it);

					break; // one reload per frame is enough
				}
			}
		}

		void VisualEngine::queueAssetReload(const std::string& filePath) {
			if (!filePath.empty())
				assetsToReload[filePath] = 2u; // wait this amount of frames before reloading
		}

		static void reloadMaterialTable(MegaClusterInstance* mci) {
			mci->reloadMaterialTable();

			if (SmoothClusterBase* sc = dynamic_cast<SmoothClusterBase*>(mci->getGfxPart()))
				sc->reloadMaterialTable();
		}

		void VisualEngine::immediateAssetReload(const std::string& filePath) {
			if (filePath.find("rbxasset://") == 0 || filePath.find("rbxgameasset://") == 0 || filePath.find("rbxapp://") == 0) {
				if (filePath == "rbxasset://terrain/materials.json") {
					if (MegaClusterInstance* mci = Instance::fastDynamicCast<MegaClusterInstance>(sceneUpdater->getDataModel()->getWorkspace()->getTerrain()))
						DataModel::get(mci)->submitTask(boost::bind(reloadMaterialTable, mci), DataModelJob::Write);
				}

				getTextureManager()->reloadImage(ContentId(filePath));
			}
			else {
				std::string extension = filePath.substr(std::min(filePath.find_last_of(".") + 1u, filePath.size()));
				if (extension == "hlsl" || extension == "hlsli") {
					StandardOut::singleton()->printf(MESSAGE_INFO, "Reloading shaders");
					reloadShaders();
				}
			}
		}

	}
}
