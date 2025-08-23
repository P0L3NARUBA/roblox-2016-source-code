#include "stdafx.h"
#include "RenderView.h"

#include "Util/FileSystem.h"
#include "g3d/GImage.h"
#include "g3d/BinaryOutput.h"
#include "V8DataModel/Lighting.h"
#include "V8DataModel/Decal.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/Sky.h"
#include "V8DataModel/ContentProvider.h"
#include "V8DataModel/MeshContentProvider.h"
#include "V8DataModel/Camera.h"
#include "V8DataModel/TextService.h"
#include "V8DataModel/RenderHooksService.h"
#include "V8DataModel/Stats.h"
#include "v8datamodel/DataModel.h"
//#include "v8datamodel/PlatformService.h"
#include "v8world/World.h"
#include "v8datamodel/UserInputService.h"

// POST EFFECT IMPORTS //
#include "V8DataModel/PostEffect.h"
#include "V8DataModel/BloomEffect.h"
#include "V8DataModel/BlurEffect.h"
#include "V8DataModel/ColorCorrectionEffect.h"

#include "util/standardout.h"
#include "rbx/MathUtil.h"
#include "VisualEngine.h"
#include "VertexStreamer.h"
#include "TypesetterBitmap.h"
#include "Water.h"

#include "SceneManager.h"
#include "SceneUpdater.h"
#include "MeshInstancer.h"

#include "TextureCompositor.h"

#include "GfxBase/RenderStats.h"
#include "GfxBase/FrameRateManager.h"
#include "util/Profiling.h"
#include "util/IMetric.h"

#include "TextureManager.h"
#include "AdornRender.h"
#include "MaterialGenerator.h"
//#include "GeometryGenerator.h"
//#include "Vertex.h"

#if !defined(RBX_PLATFORM_DURANGO)
#include "ObjectExporter.h"
#endif
#include "TextureAtlas.h"

#include "FastLog.h"

#include "Sky.h"
#include "Util.h"
#include "rbx/SystemUtil.h"

#include "GfxCore/Framebuffer.h"
#include "GfxCore/Device.h"

#include "SpatialHashedScene.h"
#include "CullableSceneNode.h"
#include "LightObject.h"

#include "rbx/rbxTime.h"

#include "rbx/Profiler.h"
#include "GfxCore/States.h"
#include "ShaderManager.h"

#include "G3D/Quat.h"

#include "GfxBase/AdornBillboarder.h"

#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
#include <mmsystem.h>
#endif

LOGGROUP(ThumbnailRender)
LOGGROUP(RenderBreakdown)
LOGGROUP(RenderStatsOnLogs)
LOGGROUP(Graphics)

FASTFLAGVARIABLE(DebugAdornsDisabled, false)

DYNAMIC_FASTFLAGVARIABLE(ScreenShotDuplicationFix, false)

FASTINTVARIABLE(OutlineBrightnessMin, 50)
FASTINTVARIABLE(OutlineBrightnessMax, 160)
FASTINTVARIABLE(OutlineThickness, 40)
FASTFLAGVARIABLE(RenderThumbModelReflectionsFix, false);

FASTINTVARIABLE(RenderShadowIntensity, 75)

FASTFLAG(UseDynamicTypesetterUTF8)
FASTFLAG(FramerateVisualizerShow)
FASTFLAG(TaskSchedulerCyclicExecutive)

// Test: 0 - no throttling, 1 - adds 5ms to frame time, 2 - adds 10ms to frame time
ABTEST_NEWUSERS_VARIABLE(FrameRateThrottling)
FASTFLAG(ClientABTestingEnabled)

FASTFLAG(TypesettersReleaseResources);

FASTFLAGVARIABLE(RenderFixFog, false)

FASTFLAGVARIABLE(RenderVR, false)

FASTFLAGVARIABLE(RenderLowLatencyLoop, false)

FASTFLAGVARIABLE(RenderUIAs3DInVR, true)

// Studs-to-meters ratio
const float kVRScale = 3.0f;

// Canvas size for 2D UI in VR
const float kVRUICanvas = 720.0f;

// Depth of 2D UI in studs
const float kVRUIDepth = 3.0f;

// Field of view of 2D UI in degrees
const float kVRUIFOV = 50.0f;

#ifdef _WIN32
extern "C"
{
	_declspec(dllexport) int NvOptimusEnablement = 1;
	_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif


namespace RBX {

	void RenderView_InitModule() {
		class RenderViewFactory : public IViewBaseFactory {
		public:
			RenderViewFactory() {
				ViewBase::RegisterFactory(CRenderSettings::Direct3D11, this);
			}

			ViewBase* Create(CRenderSettings::GraphicsMode mode, OSContext* context, CRenderSettings* renderSettings) {
				return new Graphics::RenderView(mode, context, renderSettings);
			}
		};

		static RenderViewFactory factory;
	}

	void RenderView_ShutdownModule()
	{
	}

}

namespace RBX {
	namespace Graphics {
		double busyWaitLoop(double ms);

		class FrameRateManagerStatsItem : public RBX::Stats::Item {
			uint32_t qualityLevel;
			bool autoQuality;
			uint32_t numberOfSettles;
			double averageSwitches;
			double averageFPS;

		public:
			FrameRateManagerStatsItem()
				: qualityLevel(0u)
				, autoQuality(false)
				, numberOfSettles(0u)
				, averageSwitches(0.0)
				, averageFPS(0.0)
			{
			}

			static shared_ptr<FrameRateManagerStatsItem> create() {
				shared_ptr<FrameRateManagerStatsItem> result = RBX::Creatable<RBX::Instance>::create<FrameRateManagerStatsItem>();

				result->createBoundChildItem("QualityLevel", result->qualityLevel);
				result->createBoundChildItem("AutoQuality", result->autoQuality);
				result->createBoundChildItem("NumberOfSettles", result->numberOfSettles);
				result->createBoundChildItem("AverageSwitches", result->averageSwitches);
				result->createBoundChildItem("AverageFPS", result->averageFPS);

				return result;
			}

			void updateData(FrameRateManager* frm) {
				RBX::FrameRateManager::Metrics metrics = frm->GetMetrics();

				qualityLevel = metrics.QualityLevel;
				autoQuality = metrics.AutoQuality;
				numberOfSettles = metrics.NumberOfSettles;
				averageSwitches = metrics.AverageSwitchesPerSettle;
				averageFPS = metrics.AverageFps;
			}
		};

		struct GraphicsModeTranslation {
			CRenderSettings::GraphicsMode settingsItem;
			Device::API api;
		};

		static const uint8_t validGraphicsModes = 1u;
		static const GraphicsModeTranslation gGraphicsModesTranslation[validGraphicsModes] = {
			{ CRenderSettings::Direct3D11, Device::API_Direct3D11 },
		};

		static CoordinateFrame computeVRFrame(const DeviceVR::Pose& pose) {
			CoordinateFrame result;
			result.translation = Vector3(pose.position) * kVRScale;
			result.rotation = G3D::Quat(pose.orientation).toRotationMatrix();

			return result;
		}

		static Vector2 computeCanvasSize(Device* device) {
			if (device->getVR()) {
				return Vector2(kVRUICanvas, kVRUICanvas);
			}
			else if (Framebuffer* framebuffer = device->getMainFramebuffer()) {
				uint32_t viewScale = (device->getCaps().retina) ? 2u : 1u;

				return Vector2(framebuffer->getWidth(), framebuffer->getHeight()) / viewScale;
			}
			else {
				return Vector2();
			}
		}

		RenderView::RenderView(CRenderSettings::GraphicsMode graphicsMode, OSContext* context, CRenderSettings* renderSettings)
			: deltaMs(0.0)
			, timestampMs(0.0)
			, prepareTime(0.0f)
			, totalRenderTime(0.0f)
			, prepareAverage(15.0)
			, performAverage(15.0)
			, presentAverage(15.0)
			, artificialDelay(0.0)
			, gpuAverage(15)
			, lightingValid(false)
			, doScreenshot(false)
			, adornsEnabled(true)
			, fogEndCurrentFRM(10000.0f)
			, fogEndTargetFRM(10000.0f)
			, frameDataCallback(FrameDataCallback())
		{
			FASTLOG1(FLog::ViewRbxInit, "RenderView created - %p", this);

#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
			timeBeginPeriod(1u);
#endif

			bool translationFound = false;
			for (uint8_t i = 0u; i < validGraphicsModes; ++i) {
				if (gGraphicsModesTranslation[i].settingsItem == graphicsMode) {
					device.reset(Device::create(gGraphicsModesTranslation[i].api, context->hWnd));

					translationFound = true;

					break;
				}
			}

			if (!translationFound)
				throw RBX::runtime_error("Not supported graphics mode");

			const DeviceCaps& caps = device->getCaps();

			if (!caps.supportsFramebuffer)
				throw std::runtime_error("Device does not support framebuffers");

			if (!caps.supportsShaders && !caps.supportsFFP)
				throw std::runtime_error("Device does not support shaders or FFP");

			visualEngine.reset(new VisualEngine(device.get(), renderSettings));
		}

		void RenderView::enableAdorns(bool enable) {
			adornsEnabled = enable;
		}

		void RenderView::initResources() {
			FASTLOG1(FLog::ViewRbxInit, "RenderView::initResources - %p", this);

			FASTLOG(FLog::ViewRbxInit, "RenderView::initResources finish");
		}

		void RenderView::bindWorkspace(boost::shared_ptr<RBX::DataModel> dataModel) {
			if (this->dataModel == dataModel)
				return;

			FASTLOG2(FLog::ViewRbxInit, "BindWorkspace, new datamodel %p, old datamodel: %p", dataModel.get(), this->dataModel.get());

			this->lightingValid = false;
			this->skyboxValid = false;
			this->doScreenshot = false;

			if (this->dataModel) {
				if (frameRateManagerStatsItem) {
					frameRateManagerStatsItem->setParent(nullptr);
					frameRateManagerStatsItem.reset();
				}

				lightingChangedConnection.disconnect();
				screenshotConnection.disconnect();

				if (TextService* textService = ServiceProvider::create<TextService>(this->dataModel.get())) {
					textService->clearTypesetters();
				}

				visualEngine->bindWorkspace(shared_ptr<DataModel>());
			}

			this->dataModel = dataModel;

			if (this->dataModel) {
				visualEngine->bindWorkspace(dataModel);

				Lighting* lighting = ServiceProvider::create<Lighting>(dataModel.get());
				lightingChangedConnection = lighting->lightingChangedSignal.connect(boost::bind(&RenderView::invalidateLighting, this, _1));
				screenshotConnection = this->dataModel->screenshotSignal.connect(boost::bind(&RenderView::onTakeScreenshot, this));

				if (TextService* textService = ServiceProvider::create<TextService>(dataModel.get())) {
					for (Text::Font font = Text::FONT_LEGACY; font != Text::FONT_LAST; font = Text::Font(font + 1)) {
						if (FFlag::TypesettersReleaseResources) {
							visualEngine->getTypesetter(font)->loadResources(visualEngine->getTextureManager(), visualEngine->getGlyphAtlas());
						}

						textService->registerTypesetter(TextService::FromTextFont(font), visualEngine->getTypesetter(font));
					}
				}

				FrameRateManager* frm = visualEngine->getFrameRateManager();

				if (frm) {
					frm->StartCapturingMetrics();

					RBX::Stats::StatsService* stats = RBX::ServiceProvider::create<RBX::Stats::StatsService>(this->dataModel.get());

					frameRateManagerStatsItem = FrameRateManagerStatsItem::create();
					frameRateManagerStatsItem->setName("FrameRateManager");
					frameRateManagerStatsItem->setParent(stats);
					frameRateManagerStatsItem->updateData(frm);
				}

				if (visualEngine->getDevice()->getMainFramebuffer()) {
					this->dataModel->setInitialScreenSize(computeCanvasSize(visualEngine->getDevice()));
				}

				RBX::RenderHooksService* service = RBX::ServiceProvider::find<RBX::RenderHooksService>(dataModel.get());
				service->setRenderHooks(this);

				service->reloadShadersSignal.connect(boost::bind(&RenderView::reloadShaders, this));

				// default fog
				fogEndCurrentFRM = fogEndTargetFRM = lighting->getFogDensity();

				visualEngine->getSettings()->setDrawConnectors(dataModel->isStudio());
			}
		}


		RenderView::~RenderView(void) {
			bindWorkspace(boost::shared_ptr<RBX::DataModel>());

			//sendFeatureLevelStats();

#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
			timeEndPeriod(1u);
#endif
			FASTLOG(FLog::ViewRbxInit, "RenderView destroyed");
		}

		void RenderView::onResize(uint32_t cx, uint32_t cy)
		{
		}

		FrameRateManager* RenderView::getFrameRateManager() {
			return visualEngine->getFrameRateManager();
		}

		RBX::Instance* RenderView::getWorkspace() {
			return dataModel->getWorkspace();
		}

		void RenderView::invalidateLighting(bool updateSkybox) {
			lightingValid = false;

			if (updateSkybox)
				skyboxValid = false;
		}

		void RenderView::onTakeScreenshot() {
			doScreenshot = true;
		}

		bool RenderView::getAndClearDoScreenshot() {
			bool result = doScreenshot;

			doScreenshot = false;

			return result;
		}

		void RenderView::updateLighting(Lighting* lighting) {
			SceneManager* sceneManager = visualEngine->getSceneManager();

			// Lighting
			if (!lightingValid) {
				presetLighting(lighting);

				lightingValid = true;
			}

			// Skybox
			if (!skyboxValid) {
				shared_ptr<RBX::Sky> lightingSky = lighting->sky;
				Sky* sky = sceneManager->getSky();

				if (lightingSky) {
					sky->setSkyBox(lightingSky->getSkyboxRt(), lightingSky->getSkyboxLf(), lightingSky->getSkyboxBk(), lightingSky->getSkyboxFt(), lightingSky->getSkyboxUp(), lightingSky->getSkyboxDn(), lightingSky->getSkyboxHDRI());
					sky->update(lighting->getSkyParameters(), lightingSky->getNumStars(), lightingSky->getDrawCelestialBodies(), lightingSky->getUseHDRI());
				}
				else {
					sky->setSkyBoxDefault();
					sky->update(lighting->getSkyParameters(), 3000u, true, true);
				}

				skyboxValid = true;
			}

			presetPostProcess(lighting);

			presetProcessing(lighting);
		}

		void RenderView::updateVR() {
			if (!FFlag::RenderVR)
				return;

			RBXPROFILER_SCOPE("Render", "updateVR");

			if (DeviceVR* vr = visualEngine->getDevice()->getVR()) {
				// Disable throttling for VR (ideally render job should control this...)
				TaskScheduler::singleton().DataModel30fpsThrottle = false;

				// Put FRM into aggressive mode
				visualEngine->getFrameRateManager()->setAggressivePerformance(true);

				// Read VR sensor data
				vr->update();

				DeviceVR::State vrState = vr->getState();

				// Setup interaction between DataModel and game
				if (UserInputService* uis = ServiceProvider::find<UserInputService>(dataModel.get())) {
					RBXPROFILER_SCOPE("Render", "DM");

					uis->setVREnabled(true);

					if (vrState.headPose.valid) {
						CoordinateFrame cframe = computeVRFrame(vrState.headPose);

						// Legacy, to be removed
						uis->setUserHeadCFrame(cframe);

						uis->setUserCFrame(UserInputService::USERCFRAME_HEAD, cframe);
					}

					if (vrState.handPose[0].valid) {
						CoordinateFrame cframe = computeVRFrame(vrState.handPose[0]);

						uis->setUserCFrame(UserInputService::USERCFRAME_LEFTHAND, cframe);
					}

					if (vrState.handPose[1].valid) {
						CoordinateFrame cframe = computeVRFrame(vrState.handPose[1]);

						uis->setUserCFrame(UserInputService::USERCFRAME_RIGHTHAND, cframe);
					}

					if (uis->checkAndClearRecenterUserHeadCFrameRequest())
						vr->recenter();
				}
			}
			else {
				// Reenable throttling if necessary
				TaskScheduler::singleton().DataModel30fpsThrottle = DataModel::throttleAt30Fps;

				// Put FRM into non-aggressive mode
				visualEngine->getFrameRateManager()->setAggressivePerformance(false);

				// Clean up VR state
				if (UserInputService* uis = ServiceProvider::find<UserInputService>(dataModel.get())) {
					uis->setVREnabled(false);
					uis->setUserHeadCFrame(CoordinateFrame());
				}
			}
		}

		void RenderView::enableVR(bool enabled) {
			if (!FFlag::RenderVR)
				return;

			visualEngine->getDevice()->setVR(enabled);
		}

		const char* RenderView::getVRDeviceName() {
			return device->getVR() ? "VR" : nullptr;
		}

		double RenderView::getMetricValue(const std::string& name) {
			FrameRateManager* frm = visualEngine->getFrameRateManager();

			if (name == "Delta Between Renders") {
				return frm != nullptr ? frm->GetFrameTimeAverage() : 0.0;
			}
			else if (name == "Total Render") {
				return frm != nullptr ? frm->GetRenderTimeAverage() : 0.0;
			}
			else if (name == "Present Time") {
				return presentAverage.getStats().average;
			}
			else if (name == "GPU Delay") {
				return gpuAverage.getStats().average;
			}

			return -1.0;
		}

		std::string RenderView::getRenderStatsMetric(const std::string& name) {
			if (name.compare(0u, 15u, "RenderStatsPass") == 0) {
				RenderPassStats* stats =
					(name == "RenderStatsPassScene") ? &visualEngine->getRenderStats()->passScene :
					(name == "RenderStatsPassShadow") ? &visualEngine->getRenderStats()->passShadow :
					(name == "RenderStatsPassUI") ? &visualEngine->getRenderStats()->passUI :
					(name == "RenderStatsPass3dAdorns") ? &visualEngine->getRenderStats()->pass3DAdorns :
					(name == "RenderStatsPassTotal") ? &visualEngine->getRenderStats()->passTotal :
					nullptr;

				if (!stats) return "unknown pass";

				return RBX::format("%d (%df %dv %dp %ds)",
					stats->batches, stats->faces, stats->vertices, stats->passChanges, stats->stateChanges);
			}

			if (name == "RenderStatsResolution") {
				return RBX::format("%u x %u", visualEngine->getViewWidth(), visualEngine->getViewHeight());
			}

			if (name == "RenderStatsTimeTotal") {
				FrameRateManager* frm = visualEngine->getFrameRateManager();
				double totalWork = frm->GetRenderTimeAverage();

				double prepareAve = frm->GetPrepareTimeAverage();
				double performAve = performAverage.getStats().average;
				double presentAve = presentAverage.getStats().average;

				return RBX::format("%.1f ms (prepare %.1f, perform %.1f, present %.1f, bonus %.1f)",
					totalWork,
					prepareAve,
					performAve,
					presentAve,
					artificialDelay);
			}

			if (name == "RenderStatsDelta") {
				FrameRateManager* frm = visualEngine->getFrameRateManager();
				double totalWork = frm->GetRenderTimeAverage();

				double prepareAve = frm->GetPrepareTimeAverage();
				double performAve = performAverage.getStats().average;
				double presentAve = presentAverage.getStats().average;
				double workAve = prepareAve + performAve + presentAve;

				double frameTime = frm->GetFrameTimeAverage();

				return RBX::format("%.1f ms (work %.1f, marshal %.1f, idle %.1f)",
					frameTime,
					workAve,
					workAve > totalWork ? 0.0 : totalWork - workAve,
					totalWork > frameTime ? 0.0 : frameTime - totalWork);
			}

			if (name == "RenderStatsGeometryGen") {
				RBX::RenderStats* stats = visualEngine->getRenderStats();
				//SceneUpdater* su = visualEngine->getSceneUpdater();

				return RBX::format("fast %dc %dp mega %dc queue %dc",
					stats->lastFrameFast.clusters, stats->lastFrameFast.parts,
					stats->lastFrameMegaClusterChunks,
					0);//int32_t(su->getUpdateQueueSize()));
			}

			if (name == "RenderStatsClusters") {
				RBX::RenderStats* stats = visualEngine->getRenderStats();

				return RBX::format("fw %dc %dp; dyn %dc %dp; hum %dc %dp",
					stats->clusterFastFW.clusters, stats->clusterFastFW.parts,
					stats->clusterFast.clusters, stats->clusterFast.parts,
					stats->clusterFastHumanoid.clusters, stats->clusterFastHumanoid.parts);
			}

			if (name == "RenderStatsFRMConfig") {
				FrameRateManager* frm = visualEngine->getFrameRateManager();

				return RBX::format("level %d (auto %s)",
					frm->GetQualityLevel(), visualEngine->getSettings()->getQualityLevel() == CRenderSettings::QualityAuto ? "on" : "off");
			}

			if (name == "RenderStatsFRMBlocks") {
				FrameRateManager* frm = visualEngine->getFrameRateManager();

				return RBX::format("%d (target %d)", (int)frm->GetVisibleBlockCounter(), (int)frm->GetVisibleBlockTarget());
			}

			if (name == "RenderStatsFRMDistance") {
				FrameRateManager* frm = visualEngine->getFrameRateManager();

				return RBX::format("render %d view %d (...%d)", (int)sqrt(frm->GetRenderCullSqDistance()), (int)sqrt(frm->GetViewCullSqDistance()), frm->GetRecomputeDistanceDelay());
			}

			if (name == "RenderStatsFRMAdjust") {
				FrameRateManager* frm = visualEngine->getFrameRateManager();

				return RBX::format("up %d down %d backoff %d backoff avg %d", frm->GetQualityDelayUp(), frm->GetQualityDelayDown(), frm->GetBackoffCounter(), (int)frm->GetBackoffAverage());
			}

			if (name == "RenderStatsFRMTargetTime") {
				FrameRateManager* frm = visualEngine->getFrameRateManager();

				if (frm->GetQualityLevel() > 0u && frm->GetQualityLevel() < CRenderSettings::QualityLevelMax - 1u)
					return RBX::format("frame %.1f render %1.f throttle %u", frm->GetTargetFrameTimeForNextLevel(), frm->GetTargetRenderTimeForNextLevel(), frm->getPhysicsThrottling());
				else
					return RBX::format("frame n/a render n/a throttle %u", frm->getPhysicsThrottling());
			}

			if (name == "RenderStatsTC") {
				TextureCompositor* tc = visualEngine->getTextureCompositor();

				const TextureCompositorConfiguration& config = tc->getConfiguration();
				const TextureCompositorStats& stats = tc->getStatistics();

				return RBX::format("%dM hq %d (%dM) lq %d (%dM) cache %d (%dM)",
					config.budget / 1048576u,
					stats.liveHQCount, stats.liveHQSize / 1048576u,
					stats.liveLQCount, stats.liveLQSize / 1048576u,
					stats.orphanedCount, stats.orphanedSize / 1048576u);
			}

			if (name == "RenderStatsTM") {
				const TextureManagerStats& stats = visualEngine->getTextureManager()->getStatistics();

				return RBX::format("queued %d live %d (%dM) cache %d (%dM)",
					stats.queuedCount,
					stats.liveCount, stats.liveSize / 1048576u,
					stats.orphanedCount, stats.orphanedSize / 1048576u);
			}

			if (name == "RenderStatsLightGrid") {
				/*SceneUpdater* su = visualEngine->getSceneUpdater();
				const RBX::WindowAverage<double, double>::Stats& stats = su->getLightingTimeStats();

				if (su->isLightingActive())
					return RBX::format("%d (occupancy %d, oldest: %d), %.1f ms (std: %.1f)", su->getLastLightingUpdates(), su->getLastOccupancyUpdates(), su->getLightOldestAge(), stats.average, sqrt(stats.variance));
				else*/
					return "inactive";
			}

			if (name == "RenderStatsGPU") {
				return RBX::format("%.1f ms (present: %.1f ms) %s", gpuAverage.getStats().average, presentAverage.getStats().average, visualEngine->getDevice()->getAPIName().c_str());
			}

			return "";
		}

		void RenderView::captureMetrics(RBX::RenderMetrics& metrics) {
			FrameRateManager* frm = visualEngine->getFrameRateManager();

			metrics.presentTime = presentAverage.getStats().average;
			metrics.GPUDelay = gpuAverage.getStats().average;
			metrics.deltaAve = frm->GetFrameTimeAverage();

			WindowAverage<double, double>::Stats renderStats = frm->GetRenderTimeStats().getSanitizedStats();

			metrics.renderAve = renderStats.average;
			GetConfidenceInterval(renderStats.average, renderStats.variance, C90, &metrics.renderConfidenceMin, &metrics.renderConfidenceMax);
			metrics.renderStd = sqrt(renderStats.variance);
		}

		void RenderView::printScene()
		{
		}

		void RenderView::reloadShaders() {
			visualEngine->reloadShaders();
		}

		struct ProxyMetric : IMetric {
			RenderView* view;
			IMetric* parent;

			ProxyMetric(RenderView* view, IMetric* parent)
				: view(view), parent(parent)
			{
			}

			virtual std::string getMetric(const std::string& metric) const {
				if (metric.compare(0u, 11u, "RenderStats") == 0)
					return view->getRenderStatsMetric(metric);
				else
					return parent->getMetric(metric);
			}

			virtual double getMetricValue(const std::string& metric) const {
				if (metric.compare(0u, 3u, "FRM") == 0)
					return view->getFrameRateManager()->getMetricValue(metric);
				else
					return parent->getMetricValue(metric);
			}
		};

		void RenderView::renderPrepare(IMetric* metric) {
			renderPrepareImpl(metric, /* updateViewport= */ true);
		}

		void RenderView::renderPrepareImpl(IMetric* metric, bool updateViewport) {
			RBXPROFILER_SCOPE("Render", "Prepare");

			Timer<Time::Precise> timer;

			FrameRateManager* frm = visualEngine->getFrameRateManager();

			fogEndTargetFRM = sqrt(frm->GetRenderCullSqDistance()); // filters out a glitch when manually switching FRM level

			frm->SubmitCurrentFrame(deltaMs, totalRenderTime, prepareTime, artificialDelay);

			if (dataModel->getWorkspace())
				dataModel->getWorkspace()->renderingDistance = fogEndTargetFRM;

			if (frameRateManagerStatsItem)
				frameRateManagerStatsItem->updateData(frm);

			visualEngine->reloadQueuedAssets();

			double newtimeMs = Time::now(Time::Precise).timestampSeconds() * 1000.0;
			if (timestampMs != 0.0) {
				deltaMs = (newtimeMs - timestampMs);
			}
			timestampMs = newtimeMs;

			if (!visualEngine->getDevice()->validate())
				return;

			if (updateViewport) {
				Vector2 newViewport = computeCanvasSize(visualEngine->getDevice());

				visualEngine->setViewport(newViewport.x, newViewport.y);

				if (Camera* camera = dataModel->getWorkspace()->getCamera())
				{
					// HACK: Ideally we'd update Camera viewport in renderStep since this way renderStep has data that's one frame behind
					DataModel::scoped_write_transfer request(dataModel.get());

					camera->setViewport(newViewport);
				}
			}

			dataModel->getWorkspace()->getWorld()->setFRMThrottle(frm->getPhysicsThrottling());

			Lighting* lighting = ServiceProvider::find<Lighting>(dataModel.get());
			RBXASSERT(lighting);

			visualEngine->getSceneManager()->trackLightingTimeOfDay(lighting->getGameTime());

			Workspace* workspace = dataModel->getWorkspace();
			RBX::Camera* cameraobj = workspace->getCamera();

			Vector3 poi;

			if (cameraobj->getCameraSubject()) {
				poi = cameraobj->getCameraFocus().translation;
			}
			else {
				poi = cameraobj->getCameraCoordinateFrame().translation;
			}

			visualEngine->setCamera(*cameraobj, poi);

			Adorn* adorn = visualEngine->getAdorn();

			adorn->preSubmitPass();

			if (adornsEnabled && !FFlag::DebugAdornsDisabled) {
				// HACK: Rendering code should NOT invoke Lua code or change DM in any way. But it does (in ScreenGui & Frame objects inside SurfaceGui).
				DataModel::scoped_write_transfer request(dataModel.get());

				// do 3d adorns first (they will actually be drawn after normal 3D pass)
				dataModel->renderPass3dAdorn(adorn);

				// then do 2d adorns, after 3d adorns (so that UI text shows on top)
				ProxyMetric proxyMetric(this, metric);

				if (FFlag::RenderUIAs3DInVR && adorn->isVR()) {
					Rect2D viewport = adorn->getViewport();

					CoordinateFrame cframe = cameraobj->getRenderingCoordinateFrame();

					float uiFovTan = tanf(G3D::toRadians(kVRUIFOV / 2.0f));
					float uiDepth = kVRUIDepth;
					float uiSize = uiDepth * uiFovTan * 2.0f;

					cframe.translation += cframe.lookVector() * uiDepth;
					cframe.translation -= cframe.rightVector() * uiSize * 0.5f;
					cframe.translation += cframe.upVector() * uiSize * 0.5f;
					cframe.rotation *= Matrix3::fromDiagonal(Vector3(uiSize / viewport.width(), uiSize / viewport.height(), 1.0f));

					AdornBillboarder adornView(adorn, viewport, cframe, /* alwaysOnTop= */ true);

					dataModel->renderPass2d(&adornView, &proxyMetric);
				}
				else {
					dataModel->renderPass2d(adorn, &proxyMetric);
				}
			}

			adorn->postSubmitPass();

			updateLighting(lighting);

			float dt = frm->GetFrameTimeStats().getLatest() / 1000.0f;

			visualEngine->getWater()->update(dataModel.get(), dt);

			//visualEngine->getSceneUpdater()->updatePrepare(0, *visualEngine->getUpdateFrustum());
			visualEngine->getMeshInstancer()->updateClusters();

			visualEngine->getTextureCompositor()->update(poi);

			// Pass FRM configuration to shaders
			//GlobalShaderData& globalShaderData = visualEngine->getSceneManager()->writeGlobalShaderData();

			float shadingDistance = std::max(frm->getShadingDistance(), 0.1f);
			float fovCoefficient = visualEngine->getCamera().getProjectionMatrix()[1][1];
			float outlineScaling = fovCoefficient * visualEngine->getViewHeight() * (10.0f / FInt::OutlineThickness) * (1.0f / 2.0f) / shadingDistance;

			FASTLOG(FLog::ViewRbxBase, "Render prepare end");
			prepareTime = timer.delta().msec();
			prepareAverage.sample(prepareTime);
		}

		void RenderView::drawRecordingFrame(DeviceContext* context) {
			RBXASSERT(videoFrameVertexStreamer);

			Framebuffer* fb = visualEngine->getDevice()->getMainFramebuffer();

			static const uint32_t lineWidth = 2u;
			uint32_t w = fb->getWidth();
			uint32_t h = fb->getHeight();

			Rect2D screen = Rect2D::xywh(0.0f, 0.0f, w, h);
			Rect2D paddedScreen = Rect2D::xywh(lineWidth, lineWidth, float(w - lineWidth * 2u), float(h - lineWidth * 2u));

			videoFrameVertexStreamer->rectBlt(std::shared_ptr<Texture>(), Color4(1.0f, 0.0f, 0.0f), screen.corner(3), paddedScreen.corner(3), screen.corner(0), paddedScreen.corner(0), Vector2::zero(), Vector2::zero(), BatchTextureType_Color, true);
			videoFrameVertexStreamer->rectBlt(std::shared_ptr<Texture>(), Color4(1.0f, 0.0f, 0.0f), paddedScreen.corner(2), screen.corner(2), paddedScreen.corner(1), screen.corner(1), Vector2::zero(), Vector2::zero(), BatchTextureType_Color, true);
			videoFrameVertexStreamer->rectBlt(std::shared_ptr<Texture>(), Color4(1.0f, 0.0f, 0.0f), paddedScreen.corner(0), paddedScreen.corner(1), screen.corner(0), screen.corner(1), Vector2::zero(), Vector2::zero(), BatchTextureType_Color, true);
			videoFrameVertexStreamer->rectBlt(std::shared_ptr<Texture>(), Color4(1.0f, 0.0f, 0.0f), screen.corner(3), screen.corner(2), paddedScreen.corner(3), paddedScreen.corner(2), Vector2::zero(), Vector2::zero(), BatchTextureType_Color, true);

			videoFrameVertexStreamer->renderPrepare();
			videoFrameVertexStreamer->render2D(context, w, h, visualEngine->getRenderStats()->passUI);
			videoFrameVertexStreamer->renderFinish();
		}

		void RenderView::drawVRWindow(DeviceContext* context) {
			RBXPROFILER_SCOPE("GPU", "Window");

			DeviceVR* vr = visualEngine->getDevice()->getVR();
			Framebuffer* mainFramebuffer = visualEngine->getDevice()->getMainFramebuffer();
			Framebuffer* eyeFramebuffer = vr->getEyeFramebuffer(0);

			if (!vrVertexStreamer)
				vrVertexStreamer.reset(new VertexStreamer(visualEngine.get()));

			if (!vrDebugTexture || vrDebugTexture->getWidth() != eyeFramebuffer->getWidth() || vrDebugTexture->getHeight() != eyeFramebuffer->getHeight())
				vrDebugTexture = visualEngine->getDevice()->createTexture(Texture::Type_2D, Texture::Format_RGBA8, eyeFramebuffer->getWidth(), eyeFramebuffer->getHeight(), 1, 1, Texture::Usage_Colorbuffer);

			uint32_t w = mainFramebuffer->getWidth();
			uint32_t h = mainFramebuffer->getHeight();

			uint32_t x = (w - h) / 2u;

			const float clearColor[4] = {};

			context->copyFramebuffer(eyeFramebuffer, vrDebugTexture.get());

			context->bindFramebuffer(mainFramebuffer);
			context->clearFramebuffer(DeviceContext::Buffer_Color, clearColor, 1.0f, 0u);

			vrVertexStreamer->rectBlt(vrDebugTexture, Color4(1.0f, 1.0f, 1.0f),
				Vector2(x, h), Vector2(w - x, h),
				Vector2(x, 0.0f), Vector2(w - x, 0.0f),
				Vector2(0.0f, 0.0f), Vector2(1.0f, 1.0f), BatchTextureType_Color, false);

			vrVertexStreamer->renderPrepare();
			vrVertexStreamer->render2D(context, w, h, visualEngine->getRenderStats()->passUI);
			vrVertexStreamer->renderFinish();
		}

		namespace {
			const uint32_t g_MicroProfileFontTextureX = 1024u;
			const uint32_t g_MicroProfileFontTextureY = 9u;

			const uint16_t g_MicroProfileFontDescription[] = {
				0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
				0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
				0x0ce,0x201,0x209,0x211,0x219,0x221,0x229,0x231,0x239,0x241,0x249,0x251,0x259,0x261,0x269,0x271,
				0x1b1,0x1b9,0x1c1,0x1c9,0x1d1,0x1d9,0x1e1,0x1e9,0x1f1,0x1f9,0x279,0x281,0x289,0x291,0x299,0x2a1,
				0x2a9,0x001,0x009,0x011,0x019,0x021,0x029,0x031,0x039,0x041,0x049,0x051,0x059,0x061,0x069,0x071,
				0x079,0x081,0x089,0x091,0x099,0x0a1,0x0a9,0x0b1,0x0b9,0x0c1,0x0c9,0x2b1,0x2b9,0x2c1,0x2c9,0x2d1,
				0x0ce,0x0d9,0x0e1,0x0e9,0x0f1,0x0f9,0x101,0x109,0x111,0x119,0x121,0x129,0x131,0x139,0x141,0x149,
				0x151,0x159,0x161,0x169,0x171,0x179,0x181,0x189,0x191,0x199,0x1a1,0x2d9,0x2e1,0x2e9,0x2f1,0x0ce,
				0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
				0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
				0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
				0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
				0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
				0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
				0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
				0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
			};

			const uint8_t g_MicroProfileFont[] = {
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x10,0x78,0x38,0x78,0x7c,0x7c,0x3c,0x44,0x38,0x04,0x44,0x40,0x44,0x44,0x38,0x78,
				0x38,0x78,0x38,0x7c,0x44,0x44,0x44,0x44,0x44,0x7c,0x00,0x00,0x40,0x00,0x04,0x00,
				0x18,0x00,0x40,0x10,0x08,0x40,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x10,0x38,0x7c,0x08,0x7c,0x1c,0x7c,0x38,0x38,
				0x10,0x28,0x28,0x10,0x00,0x20,0x10,0x08,0x10,0x10,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x04,0x00,0x20,0x38,0x38,0x70,0x00,0x1c,0x10,0x00,0x1c,0x10,0x70,0x30,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x28,0x44,0x44,0x44,0x40,0x40,0x40,0x44,0x10,0x04,0x48,0x40,0x6c,0x44,0x44,0x44,
				0x44,0x44,0x44,0x10,0x44,0x44,0x44,0x44,0x44,0x04,0x00,0x00,0x40,0x00,0x04,0x00,
				0x24,0x00,0x40,0x00,0x00,0x40,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x44,0x30,0x44,0x04,0x18,0x40,0x20,0x04,0x44,0x44,
				0x10,0x28,0x28,0x3c,0x44,0x50,0x10,0x10,0x08,0x54,0x10,0x00,0x00,0x00,0x04,0x00,
				0x00,0x08,0x00,0x10,0x44,0x44,0x40,0x40,0x04,0x28,0x00,0x30,0x10,0x18,0x58,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x44,0x44,0x40,0x44,0x40,0x40,0x40,0x44,0x10,0x04,0x50,0x40,0x54,0x64,0x44,0x44,
				0x44,0x44,0x40,0x10,0x44,0x44,0x44,0x28,0x28,0x08,0x00,0x38,0x78,0x3c,0x3c,0x38,
				0x20,0x38,0x78,0x30,0x18,0x44,0x10,0x6c,0x78,0x38,0x78,0x3c,0x5c,0x3c,0x3c,0x44,
				0x44,0x44,0x44,0x44,0x7c,0x00,0x4c,0x10,0x04,0x08,0x28,0x78,0x40,0x08,0x44,0x44,
				0x10,0x00,0x7c,0x50,0x08,0x50,0x00,0x20,0x04,0x38,0x10,0x00,0x00,0x00,0x08,0x10,
				0x10,0x10,0x7c,0x08,0x08,0x54,0x40,0x20,0x04,0x44,0x00,0x30,0x10,0x18,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x44,0x78,0x40,0x44,0x78,0x78,0x40,0x7c,0x10,0x04,0x60,0x40,0x54,0x54,0x44,0x78,
				0x44,0x78,0x38,0x10,0x44,0x44,0x54,0x10,0x10,0x10,0x00,0x04,0x44,0x40,0x44,0x44,
				0x78,0x44,0x44,0x10,0x08,0x48,0x10,0x54,0x44,0x44,0x44,0x44,0x60,0x40,0x10,0x44,
				0x44,0x44,0x28,0x44,0x08,0x00,0x54,0x10,0x18,0x18,0x48,0x04,0x78,0x10,0x38,0x3c,
				0x10,0x00,0x28,0x38,0x10,0x20,0x00,0x20,0x04,0x10,0x7c,0x00,0x7c,0x00,0x10,0x00,
				0x00,0x20,0x00,0x04,0x10,0x5c,0x40,0x10,0x04,0x00,0x00,0x60,0x10,0x0c,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x7c,0x44,0x40,0x44,0x40,0x40,0x4c,0x44,0x10,0x04,0x50,0x40,0x44,0x4c,0x44,0x40,
				0x54,0x50,0x04,0x10,0x44,0x44,0x54,0x28,0x10,0x20,0x00,0x3c,0x44,0x40,0x44,0x7c,
				0x20,0x44,0x44,0x10,0x08,0x70,0x10,0x54,0x44,0x44,0x44,0x44,0x40,0x38,0x10,0x44,
				0x44,0x54,0x10,0x44,0x10,0x00,0x64,0x10,0x20,0x04,0x7c,0x04,0x44,0x20,0x44,0x04,
				0x10,0x00,0x7c,0x14,0x20,0x54,0x00,0x20,0x04,0x38,0x10,0x10,0x00,0x00,0x20,0x10,
				0x10,0x10,0x7c,0x08,0x10,0x58,0x40,0x08,0x04,0x00,0x00,0x30,0x10,0x18,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x44,0x44,0x44,0x44,0x40,0x40,0x44,0x44,0x10,0x44,0x48,0x40,0x44,0x44,0x44,0x40,
				0x48,0x48,0x44,0x10,0x44,0x28,0x6c,0x44,0x10,0x40,0x00,0x44,0x44,0x40,0x44,0x40,
				0x20,0x3c,0x44,0x10,0x08,0x48,0x10,0x54,0x44,0x44,0x44,0x44,0x40,0x04,0x12,0x4c,
				0x28,0x54,0x28,0x3c,0x20,0x00,0x44,0x10,0x40,0x44,0x08,0x44,0x44,0x20,0x44,0x08,
				0x00,0x00,0x28,0x78,0x44,0x48,0x00,0x10,0x08,0x54,0x10,0x10,0x00,0x00,0x40,0x00,
				0x10,0x08,0x00,0x10,0x00,0x40,0x40,0x04,0x04,0x00,0x00,0x30,0x10,0x18,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x44,0x78,0x38,0x78,0x7c,0x40,0x3c,0x44,0x38,0x38,0x44,0x7c,0x44,0x44,0x38,0x40,
				0x34,0x44,0x38,0x10,0x38,0x10,0x44,0x44,0x10,0x7c,0x00,0x3c,0x78,0x3c,0x3c,0x3c,
				0x20,0x04,0x44,0x38,0x48,0x44,0x38,0x44,0x44,0x38,0x78,0x3c,0x40,0x78,0x0c,0x34,
				0x10,0x6c,0x44,0x04,0x7c,0x00,0x38,0x38,0x7c,0x38,0x08,0x38,0x38,0x20,0x38,0x70,
				0x10,0x00,0x28,0x10,0x00,0x34,0x00,0x08,0x10,0x10,0x00,0x20,0x00,0x10,0x00,0x00,
				0x20,0x04,0x00,0x20,0x10,0x3c,0x70,0x00,0x1c,0x00,0x7c,0x1c,0x10,0x70,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x38,0x00,0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x40,0x04,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			};
		}

		class ProfilerRenderer : public Profiler::Renderer {
		public:
			ProfilerRenderer(VisualEngine* visualEngine)
				: visualEngine(visualEngine)
				, colorOrderBGR(false)
			{
				Device* device = visualEngine->getDevice();

				colorOrderBGR = device->getCaps().colorOrderBGR;

				std::vector<VertexLayout::Element> elements;
				/*elements.push_back(VertexLayout::Element(0u, 0u, VertexLayout::Format_Float3, VertexLayout::Input_Vertex, VertexLayout::Semantic_Position));
				elements.push_back(VertexLayout::Element(0u, 12u, VertexLayout::Format_Float2, VertexLayout::Input_Vertex, VertexLayout::Semantic_Texture));
				elements.push_back(VertexLayout::Element(0u, 20u, VertexLayout::Format_Float4, VertexLayout::Input_Vertex, VertexLayout::Semantic_Color));*/

				layout = device->createVertexLayout(elements);

				const uint32_t fontSize = g_MicroProfileFontTextureX * g_MicroProfileFontTextureY * 4u;

				uint32_t* pUnpacked = (uint32_t*)alloca(fontSize);

				uint32_t idx = 0u;
				uint32_t end = g_MicroProfileFontTextureX * g_MicroProfileFontTextureY / 8u;

				for (uint32_t i = 0u; i < end; i++) {
					uint8_t pValue = g_MicroProfileFont[i];

					for (uint32_t j = 0u; j < 8u; ++j) {
						pUnpacked[idx++] = (pValue & 0x80u) ? 0xffffffff : 0u;
						pValue <<= 1;
					}
				}

				pUnpacked[idx - 1u] = 0xffffffff;

				fontTexture = device->createTexture(Texture::Type_2D, Texture::Format_RGBA8, g_MicroProfileFontTextureX, g_MicroProfileFontTextureY, 1u, 1u, Texture::Usage_Static);
				fontTexture->upload(0u, 0u, TextureRegion(0u, 0u, g_MicroProfileFontTextureX, g_MicroProfileFontTextureY), pUnpacked, fontSize);
			}

			void drawText(int32_t x, int32_t y, uint32_t color, const char* text, uint32_t length, uint32_t textWidth, uint32_t textHeight) override {
				const float fOffsetU = float(textWidth) / float(g_MicroProfileFontTextureX);

				float fX = (float)x;
				float fY = (float)y;
				float fY2 = fY + (textHeight + 1);

				const char* pStr = text;

				if (!colorOrderBGR)
					color = 0xff000000 | ((color & 0xff) << 16) | (color & 0xff00) | ((color >> 16) & 0xff);

				for (uint32_t j = 0u; j < length; ++j) {
					int16_t nOffset = g_MicroProfileFontDescription[uint8_t(*pStr++)];
					float fOffset = float(nOffset) / float(g_MicroProfileFontTextureX);

					Vertex v0 = { fX, fY, fOffset, 0.0f, color };
					Vertex v1 = { fX + textWidth, fY, fOffset + fOffsetU, 0.0f, color };
					Vertex v2 = { fX + textWidth, fY2, fOffset + fOffsetU, 1.0f, color };
					Vertex v3 = { fX, fY2, fOffset, 1.0f, color };

					quads.push_back(v0);
					quads.push_back(v1);
					quads.push_back(v3);
					quads.push_back(v1);
					quads.push_back(v2);
					quads.push_back(v3);

					fX += textWidth + 1u;
				}
			}

			void drawBox(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color0, uint32_t color1) override {
				if (!colorOrderBGR) {
					color0 = ((color0 & 0xffu) << 16u) | ((color0 >> 16u) & 0xffu) | (0xff00ff00 & color0);
					color1 = ((color1 & 0xffu) << 16u) | ((color1 >> 16u) & 0xffu) | (0xff00ff00 & color1);
				}

				Vertex v0 = { (float)x0, (float)y0, 2.0f, 2.0f, color0 };
				Vertex v1 = { (float)x1, (float)y0, 2.0f, 2.0f, color0 };
				Vertex v2 = { (float)x1, (float)y1, 2.0f, 2.0f, color1 };
				Vertex v3 = { (float)x0, (float)y1, 2.0f, 2.0f, color1 };

				quads.push_back(v0);
				quads.push_back(v1);
				quads.push_back(v3);
				quads.push_back(v1);
				quads.push_back(v2);
				quads.push_back(v3);
			}

			void drawLine(uint32_t vertexCount, const float* vertexData, uint32_t color) override {
				if (!colorOrderBGR)
					color = ((color & 0xffu) << 16u) | ((color >> 16u) & 0xffu) | (0xff00ff00 & color);

				for (uint32_t i = 0u; i + 1u < vertexCount; ++i) {
					Vertex v0 = { vertexData[2u * i + 0u], vertexData[2u * i + 1u], 2.0f, 2.0f, color };
					Vertex v1 = { vertexData[2u * i + 2u], vertexData[2u * i + 3u], 2.0f, 2.0f, color };

					lines.push_back(v0);
					lines.push_back(v1);
				}
			}

			void flush(DeviceContext* context, const RenderCamera& camera) {
				if (quads.empty() && lines.empty())
					return;

				updateBuffer(quadsVB, quadsGeometry, quads);
				updateBuffer(linesVB, linesGeometry, lines);

				if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("ProfilerVS", "ProfilerFS")) {
					GlobalShaderData shaderData = visualEngine->getSceneManager()->readGlobalShaderData();

					shaderData.setCamera(camera);

					context->updateGlobalConstantData(&shaderData, sizeof(GlobalShaderData));

					context->bindProgram(program.get());

					context->setRasterizerState(RasterizerState::Cull_None);
					context->setBlendState(BlendState::Mode_AlphaBlend);
					context->setDepthState(DepthState(DepthState::Function_Always, false));

					context->bindTexture(0u, fontTexture);

					if (!quads.empty())
						context->draw(quadsGeometry.get(), Geometry::Primitive_Triangles, 0u, quads.size(), 0u);

					if (!lines.empty())
						context->draw(linesGeometry.get(), Geometry::Primitive_Lines, 0u, lines.size(), 0u);
				}

				quads.clear();
				lines.clear();
			}

		private:
			struct Vertex {
				float x, y;
				float u, v;
				uint32_t color;
			};

			VisualEngine* visualEngine;

			bool colorOrderBGR;

			std::vector<Vertex> quads;
			std::vector<Vertex> lines;

			std::shared_ptr<Texture> fontTexture;

			std::shared_ptr<VertexLayout> layout;

			std::shared_ptr<VertexBuffer> quadsVB;
			std::shared_ptr<Geometry> quadsGeometry;

			std::shared_ptr<VertexBuffer> linesVB;
			std::shared_ptr<Geometry> linesGeometry;

			void updateBuffer(std::shared_ptr<VertexBuffer>& vb, std::shared_ptr<Geometry>& geometry, const std::vector<Vertex>& vertices) {
				if (vertices.empty()) return;

				if (!vb || vb->getElementCount() < vertices.size()) {
					size_t count = 1u;
					while (count < vertices.size())
						count *= 2u;

					vb = visualEngine->getDevice()->createVertexBuffer(sizeof(Vertex), count, GeometryBuffer::Usage_Dynamic);
					geometry = visualEngine->getDevice()->createGeometry(layout, vb, std::shared_ptr<IndexBuffer>());
				}

				void* locked = vb->lock(VertexBuffer::Lock_Discard);
				memcpy(locked, vertices.data(), vertices.size() * sizeof(Vertex));
				vb->unlock();
			}
		};

		void RenderView::drawProfiler(DeviceContext* context) {
			if (!Profiler::isVisible())
				return;

			if (!profilerRenderer)
				profilerRenderer.reset(new ProfilerRenderer(visualEngine.get()));

			RBXPROFILER_SCOPE("GPU", "Profiler");

			Framebuffer* fb = visualEngine->getDevice()->getMainFramebuffer();

			Profiler::render(profilerRenderer.get(), fb->getWidth(), fb->getHeight());

			RenderCamera camera;
			camera.setViewMatrix(Matrix4::identity());
			camera.setProjectionOrtho(fb->getWidth(), fb->getHeight(), -1.0f, 1.0f, visualEngine->getDevice()->getCaps().needsHalfPixelOffset);

			profilerRenderer->flush(context, camera);
		}

		void RenderView::renderPerform(double timeJobStart) {
			renderPerformImpl(timeJobStart, visualEngine->getDevice()->getMainFramebuffer());
		}

		void RenderView::renderPerformImpl(double timeJobStart, Framebuffer* mainFramebuffer) {
			RBXPROFILER_SCOPE("Render", "Perform");

			if (!this->dataModel)
				return;

			Timer<Time::Precise> timer;

			FASTLOG(FLog::ViewRbxBase, "Render perform start");

			double performTime = 0.0;
			double presentTime = 0.0;

			Adorn* adorn = visualEngine->getAdorn();

			// update glyph atlas
			if (FFlag::UseDynamicTypesetterUTF8)
				visualEngine->getGlyphAtlas()->upload();

			adorn->prepareRenderPass();

			if (DeviceContext* context = visualEngine->getDevice()->beginFrame()) {
				//visualEngine->getSceneUpdater()->updatePerform();

				visualEngine->getMaterialGenerator()->garbageCollectIncremental();
				visualEngine->getTextureManager()->garbageCollectIncremental();

				visualEngine->getTextureManager()->processPendingRequests();

				context->setDefaultAnisotropy(std::max(1u, visualEngine->getFrameRateManager()->getTextureAnisotropy()));

				visualEngine->getTextureCompositor()->render(context);

				if (DeviceVR* vr = visualEngine->getDevice()->getVR()) {
					RBXPROFILER_SCOPE("GPU", "Scene");

					DeviceVR::State vrState = vr->getState();

					const RenderCamera& cullCamera = visualEngine->getCameraCull();

					float zNear = 2147483647.0f;
					float zFar = 0.1f;

					visualEngine->getSceneManager()->renderBegin(context, cullCamera, vr->getEyeFramebuffer(0)->getWidth(), vr->getEyeFramebuffer(0)->getHeight());

					for (size_t eye = 0u; eye < 2u; ++eye) {
						RBXPROFILER_SCOPE("GPU", "Eye");

						Framebuffer* eyeFB = vr->getEyeFramebuffer(eye);

						Vector3 eyeOffset = Vector3(vrState.eyeOffset[eye]) * kVRScale;

						RenderCamera eyeCamera = visualEngine->getCamera();

						eyeCamera.setViewMatrix(Matrix4::translation(-eyeOffset) * eyeCamera.getViewMatrix());
						eyeCamera.setProjectionPerspective(vrState.eyeFov[eye][0], vrState.eyeFov[eye][1], vrState.eyeFov[eye][2], vrState.eyeFov[eye][3], zNear, zFar);

						visualEngine->getSceneManager()->renderView(context, eyeFB, eyeCamera, eyeFB->getWidth(), eyeFB->getHeight());
					}

					visualEngine->getSceneManager()->renderEnd(context);

					if (vrState.needsMirror)
						drawVRWindow(context);
				}
				else {
					visualEngine->getSceneManager()->renderScene(context, mainFramebuffer, visualEngine->getCamera(), visualEngine->getViewWidth(), visualEngine->getViewHeight());
				}

				if (getAndClearDoScreenshot()) {
					std::string screenshotFilename;

					if (saveScreenshotToFile(screenshotFilename)) {
						if (dataModel) {
							dataModel->submitTask(boost::bind(&RBX::DataModel::ScreenshotReadyTask, weak_ptr<RBX::DataModel>(dataModel), screenshotFilename), RBX::DataModelJob::Write);
						}
					}
				}

				performTime = timer.reset().msec();
				performAverage.sample(performTime);

				if (videoFrameVertexStreamer && frameDataCallback) {
					frameDataCallback(visualEngine->getDevice());
					// this has to be checked again, as frameDataCallback can stop video recording and NULL it
					if (videoFrameVertexStreamer)
						drawRecordingFrame(context);
				}

				drawProfiler(context);

				RBXPROFILER_SCOPE("Render", "Present");

				visualEngine->getDevice()->endFrame();

				presentTime = timer.delta().msec();
				presentAverage.sample(presentTime);

				gpuAverage.sample(visualEngine->getDevice()->getStatistics().gpuFrameTime);
			}

			adorn->finishRenderPass();

			totalRenderTime = (Time::nowFastSec() - timeJobStart) * 1000.0;

			FASTLOG4F(FLog::ViewRbxBase, "Render perform end. Delta: %f, Total %f, Present: %f, Prepare: %f", deltaMs, totalRenderTime, presentTime, prepareTime);
		}

		void RenderView::buildGui(bool buildInGameGui) {
			dataModel->startCoreScripts(buildInGameGui);
		}

		RenderStats& RenderView::getRenderStats() { return *visualEngine->getRenderStats(); }

		extern float bloomIntensity = 0.0f;
		extern uint32_t bloomSize = 0u;

		extern float blurIntensity = 0.0f;

		extern float brightness = 1.0f;
		extern float contrast = 1.0f;
		extern float grayscaleLevel = 1.0f;
		extern Color3 tintColor = Color3(1.0f, 1.0f, 1.0f);
		extern TonemapperMode tonemapper = ACES;

		void RenderView::presetLighting(RBX::Lighting* lighting) {
			SceneManager* sceneManager = visualEngine->getSceneManager();

			Vector3 topColorShift = Color3::toHSV(lighting->getSunColorShift());
			topColorShift.y = topColorShift.y * topColorShift.z;
			topColorShift.z = 1.0f;

			const G3D::LightingParameters& skyParameters = lighting->getSkyParameters();

			Vector3 keyDirection = -skyParameters.lightDirection.unit();
			Color3 keyColor = skyParameters.lightColor * Color3::fromHSV(topColorShift);

			Color3 ambientColor = lighting->getGlobalAmbient();
			Color3 outdoorAmbientColor = lighting->getGlobalOutdoorAmbient();

			Color3 keyLightColor = keyColor * lighting->getSunBrightness();

			sceneManager->setLighting(ambientColor, outdoorAmbientColor, keyDirection, keyLightColor);
			sceneManager->setFog(lighting->getFogColor(), lighting->getFogDensity(), lighting->getFogSunInfluence(), lighting->getFogSkybox(), lighting->getFogAffectSkybox());
		}

		void RenderView::presetPostProcess(RBX::Lighting* lighting) {
			SceneManager* sceneManager = visualEngine->getSceneManager();

			grayscaleLevel = 1.0f;
			contrast = 1.0f;
			brightness = 1.0f;
			tintColor = Color3(1.0f, 1.0f, 1.0f);
			tonemapper = ACES;

			bloomIntensity = 0.0f;
			bloomSize = 0u;

			blurIntensity = 0.0f;

			// really feel like there's a better way to do this
			if (shared_ptr<const Instances> children = lighting->getChildren2()) {
				Instances::const_iterator end = children->end();

				for (Instances::const_iterator iter = children->begin(); iter != end; ++iter) {
					std::string className = (*iter)->getClassNameStr();

					if (className == "BloomEffect") {
						auto bloomEffectInstance = boost::dynamic_pointer_cast<BloomEffect>(*iter);

						if (bloomEffectInstance && bloomEffectInstance->getEnabled())
							bloomIntensity = bloomEffectInstance->getIntensity();
							bloomSize = bloomEffectInstance->getSize();
					}
					else if (className == "BlurEffect") {
						auto blurEffectInstance = boost::dynamic_pointer_cast<BlurEffect>(*iter);

						if (blurEffectInstance && blurEffectInstance->getEnabled())
							blurIntensity = blurEffectInstance->getSize();
					}
					else if (className == "ColorCorrectionEffect") {
						auto colorCorrectionInstance = boost::dynamic_pointer_cast<ColorCorrectionEffect>(*iter);

						if (colorCorrectionInstance && colorCorrectionInstance->getEnabled()) {
							tintColor = colorCorrectionInstance->getTintColor();
							grayscaleLevel = colorCorrectionInstance->getSaturation();
							brightness = colorCorrectionInstance->getBrightness();
							contrast = colorCorrectionInstance->getContrast();
						}
					}
				}
			}

			tonemapper = lighting->getCameraTonemapper();

			sceneManager->setPostProcess(brightness, contrast, grayscaleLevel, tintColor, tonemapper, blurIntensity, bloomIntensity, bloomSize);
		}

		void RenderView::presetProcessing(RBX::Lighting* lighting) {
			SceneManager* sceneManager = visualEngine->getSceneManager();

			sceneManager->setProcessing(std::pow(2.0f, lighting->getCameraExposure()), 2.2f);
		}

		static void waitForContent(RBX::ContentProvider* contentProvider) {
			boost::xtime expirationTime;
			boost::xtime_get(&expirationTime, boost::TIME_UTC_);
			expirationTime.sec += static_cast<boost::xtime::xtime_sec_t>(120);

			// TODO: Bail out after a certain number of seconds...
			while (!contentProvider->isRequestQueueEmpty()) {
				boost::xtime xt;
				boost::xtime_get(&xt, boost::TIME_UTC_);

				if (xtime_cmp(xt, expirationTime) == 1)
					throw RBX::runtime_error("Timeout while waiting for content - 120 seconds");

				boost::this_thread::sleep(boost::posix_time::milliseconds(20));
			}
		}

		static void modifyThumbnailCamera(VisualEngine* visualEngine, bool allowDolly, uint32_t recdepth = 1u) {
			if (recdepth > 10u)
				return;

			std::vector<CullableSceneNode*> nodes = visualEngine->getSceneManager()->getSpatialHashedScene()->getAllNodes();

			Extents extents;

			for (size_t i = 0u; i < nodes.size(); ++i)
				if (!dynamic_cast<LightObject*>(nodes[i]))
					extents.expandToContain(nodes[i]->getWorldBounds());

			Matrix4 viewProj = visualEngine->getCamera().getViewProjectionMatrix();

			Extents screen;

			for (size_t i = 0u; i < 8u; ++i) {
				Vector4 p = viewProj * Vector4(extents.getCorner(i), 1.0f);
				Vector3 q = p.xyz() / p.w;

				if (p.w <= 0.001f || fabsf(q.x) > 1.0f || fabsf(q.y) > 1.0f || fabsf(q.z) > 1.0f) {
					if (!allowDolly)
						return;

					Matrix4 view = visualEngine->getCameraMutable().getViewMatrix();
					Matrix4 dolly = Matrix4::translation(0.0f, 0.0f, -1.0f - recdepth * recdepth);
					visualEngine->getCameraMutable().setViewMatrix(dolly * view);
					return modifyThumbnailCamera(visualEngine, true, recdepth + 1u); // try once again
				}

				screen.expandToContain(q);
			}

			float zoomH = 2.0f / (screen.max().x - screen.min().x);
			float zoomV = 2.0f / (screen.max().y - screen.min().y);
			float zoom = std::min(zoomV, zoomH);

			float offH = screen.center().x;
			float offV = screen.center().y;

			Matrix4 crop = Matrix4::identity();

			crop[0][0] = zoom;
			crop[1][1] = zoom;
			crop[0][3] = -zoom * offH;
			crop[1][3] = -zoom * offV;

			visualEngine->getCameraMutable().setProjectionMatrix(crop * visualEngine->getCamera().getProjectionMatrix());
		}

		void RenderView::prepareSceneGraph() {
			// will populate scenegraph, and trigger resource loads.
			//visualEngine->getSceneUpdater()->setComputeLightingEnabled(false);

			// wait for all networked resources to load.
			// do this outside scoped lock because this is the IO/network bound task.

			RBX::ContentProvider* contentProvider = visualEngine->getContentProvider();
			RBX::MeshContentProvider* meshContentProvider = visualEngine->getMeshContentProvider();
			bool allContentLoaded = false;

			do {
				FASTLOG(FLog::ThumbnailRender, "Waiting for content to load");

				waitForContent(contentProvider);

				RBX::ServiceProvider::find<RBX::Workspace>(dataModel.get())->assemble();

				renderPrepareImpl(nullptr, /* updateViewport= */ false);

				// Clear adorn rendering queue
				visualEngine->getAdorn()->finishRenderPass();

				// Load textures
				while (!visualEngine->getTextureManager()->isQueueEmpty()) {
					visualEngine->getTextureManager()->processPendingRequests();

					boost::this_thread::sleep(boost::posix_time::milliseconds(20));
				}

				// Run texture compositor
				if (!visualEngine->getTextureCompositor()->isQueueEmpty()) {
					if (DeviceContext* context = visualEngine->getDevice()->beginFrame())
					{
						visualEngine->getTextureCompositor()->render(context);
						visualEngine->getDevice()->endFrame();
					}
				}

				allContentLoaded =
					contentProvider->isRequestQueueEmpty() &&
					meshContentProvider->isRequestQueueEmpty() &&
					!visualEngine->getSceneUpdater()->arePartsWaitingForAssets() &&
					visualEngine->getTextureCompositor()->isQueueEmpty() &&
					visualEngine->getTextureManager()->isQueueEmpty();

				FASTLOG2(FLog::ThumbnailRender, "After render: Providers requests empty. Content: %u, Mesh: %u",
					contentProvider->isRequestQueueEmpty(), meshContentProvider->isRequestQueueEmpty());

				FASTLOG3(FLog::ThumbnailRender, "Parts not waiting for assets: %u, Texture Compositor Queue Empty: %u, Texture Manager Queue Empty: %u",
					!visualEngine->getSceneUpdater()->arePartsWaitingForAssets(), visualEngine->getTextureCompositor()->isQueueEmpty(), visualEngine->getTextureManager()->isQueueEmpty());
			} while (!allContentLoaded);

			//visualEngine->getSceneUpdater()->setComputeLightingEnabled(true);
		}

		void RenderView::renderThumb(unsigned char* data, uint32_t width, uint32_t height, bool crop, bool allowDolly) {
			// will populate scenegraph, and trigger resource loads.
			FASTLOG(FLog::ThumbnailRender, "Rendering thumbnail: populating scene graph, trigger resource load");
			prepareSceneGraph();

			visualEngine->getFrameRateManager()->SubmitCurrentFrame(1000.0, 1000.0, 1000.0, 0.0);

			// Run find visible objects once to fill in distance for LightGrid
			visualEngine->getSceneManager()->computeMinimumSqDistance(visualEngine->getCamera());

			visualEngine->setViewport(width, height);

			if (Camera* camera = dataModel->getWorkspace()->getCamera())
				camera->setViewport(Vector2int16(width, height));

			renderPrepareImpl(nullptr, /* updateViewport= */ false);

			if (crop)
				modifyThumbnailCamera(visualEngine.get(), allowDolly);

			std::shared_ptr<Renderbuffer> color = visualEngine->getDevice()->createRenderbuffer(Texture::Format_RGBA16f, width, height, 1u);
			std::shared_ptr<Renderbuffer> depth = visualEngine->getDevice()->createRenderbuffer(Texture::Format_D32f, width, height, 1u);
			std::shared_ptr<Framebuffer> framebuffer = visualEngine->getDevice()->createFramebuffer(color, depth);

			renderPerformImpl(0.0f, framebuffer.get());

			framebuffer->download(data, width * height * 4u);
		}

		//
		//	Write render target to file, using user picture folder\Roblox 
		//
		bool RenderView::saveScreenshotToFile(std::string& /*output*/ filename) {
#if defined(RBX_PLATFORM_DURANGO)
			// TODO: implement screenshot for durango
#else
			Framebuffer* framebuffer = visualEngine->getDevice()->getMainFramebuffer();

			if (framebuffer) {
				time_t ctTime;
				time(&ctTime);

				struct tm* pTime = localtime(&ctTime);

				std::ostringstream name;
				name << "RobloxScreenShot";
				name << std::setw(2) << std::setfill('0') << (pTime->tm_mon + 1)
					<< std::setw(2) << std::setfill('0') << pTime->tm_mday
					<< std::setw(2) << std::setfill('0') << (pTime->tm_year + 1900)
					<< "_" << std::setw(2) << std::setfill('0') << pTime->tm_hour
					<< std::setw(2) << std::setfill('0') << pTime->tm_min
					<< std::setw(2) << std::setfill('0') << pTime->tm_sec
					<< std::setw(3) << std::setfill('0') << ((clock() * 1000 / CLOCKS_PER_SEC) % 1000);
				name << ".png";

				boost::filesystem::path path = FileSystem::getUserDirectory(true, RBX::DirPicture) / name.str();

				G3D::GImage image(framebuffer->getWidth(), framebuffer->getHeight(), 4u);
				framebuffer->download(image.byte(), image.width() * image.height() * 4u);

				image.convertToRGB();

				G3D::BinaryOutput binaryOutput;
				image.encode(G3D::GImage::PNG, binaryOutput);

				std::ofstream out(path.native().c_str(), std::ios::out | std::ios::binary);
				out.write(reinterpret_cast<const char*>(binaryOutput.getCArray()), binaryOutput.length());

				// Not Unicode-safe! :(
				filename = path.string();

				return true;
			}
#endif
			return false;
		}

		void RenderView::queueAssetReload(const std::string& filePath) {
			visualEngine->queueAssetReload(filePath);
		}

		void RenderView::immediateAssetReload(const std::string& filePath) {
			visualEngine->immediateAssetReload(filePath);
		}

		bool RenderView::exportSceneThumbJSON(ExporterSaveType saveType, ExporterFormat format, bool encodeBase64, std::string& strOut) {
#if !defined(RBX_PLATFORM_DURANGO)
			FASTLOG(FLog::ThumbnailRender, "Rendering thumbnail: populating scene graph, trigger resource load");
			prepareSceneGraph();

			return ObjectExporter::exportToJSON(saveType, format, dataModel.get(), visualEngine.get(), encodeBase64, &strOut);
#else
			return false;
#endif
		}

		bool RenderView::exportScene(const std::string& filePath, ExporterSaveType saveType, ExporterFormat format) {
#if !defined(RBX_PLATFORM_DURANGO)
			return ObjectExporter::exportToFile(filePath, saveType, format, dataModel.get(), visualEngine.get());
#else
			return false;
#endif
		}

		void RenderView::suspendView() {
			device->suspend();
		}

		void RenderView::resumeView() {
			device->resume();
		}

		void RenderView::garbageCollect() {
			visualEngine->getMaterialGenerator()->garbageCollectFull();
			visualEngine->getTextureCompositor()->garbageCollectFull();
			visualEngine->getTextureManager()->garbageCollectFull();

			if (RBX::ContentProvider* contentProvider = visualEngine->getContentProvider()) {
				contentProvider->clearContent();
			}

			if (RBX::MeshContentProvider* meshContentProvider = visualEngine->getMeshContentProvider()) {
				meshContentProvider->clearContent();
			}
		}

		std::pair<uint32_t, uint32_t> RenderView::setFrameDataCallback(const boost::function<void(void*)>& callback) {
			Framebuffer* framebuffer = visualEngine->getDevice()->getMainFramebuffer();

			if (callback && framebuffer) {
				videoFrameVertexStreamer.reset(new VertexStreamer(visualEngine.get()));
				frameDataCallback = callback;
				return std::make_pair(framebuffer->getWidth(), framebuffer->getHeight());
			}
			else {
				videoFrameVertexStreamer.reset();
				frameDataCallback = FrameDataCallback();
				return std::make_pair(0u, 0u);
			}
		}

		double busyWaitLoop(double ms) {
			double start = Time::now(Time::Precise).timestampSeconds() * 1000.0;

			for (;;) {
				double cur = Time::now(Time::Precise).timestampSeconds() * 1000.0;

				if (cur - start > ms)
					return cur - start;
			}
		}

	}
}