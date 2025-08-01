#pragma once

#include <vector>
#pragma warning (push)
#pragma warning( disable:4996 ) // disable -D_SCL_SECURE_NO_WARNING in ublas. 
#include <boost/numeric/ublas/vector.hpp>
#pragma warning (pop)

#include "GfxBase/RenderSettings.h"
#include "rbx/RunningAverage.h"
#include <map>

namespace RBX {
	class RenderCaps;
	class Log;

	enum SSAOLevel {
		ssaoNone = 0,
		ssaoFullBlank,
		ssaoFull
	};

	struct THROTTLE_LOCKSTEP;

	class FrameRateManager {
	public:
		FrameRateManager(void);
		~FrameRateManager(void);

		void configureFrameRateManager(CRenderSettings::FrameRateManagerMode mode, bool hasCharacter);
		void setAggressivePerformance(bool value);

		struct Metrics {
			bool AutoQuality;
			uint32_t QualityLevel;
			uint32_t NumberOfSettles;
			double AverageSwitchesPerSettle;
			double AverageFps;
		};

		// add to current frame counter.
		void AddBlockQuota(uint32_t blocksInCluster, float sqDistanceToCamera, bool isInSpatialHash);

		bool getGBufferSetting();

		SSAOLevel getSSAOLevel();
		bool isSSAOSupported() { return mSSAOSupported; }

		float getShadingDistance() const;
		float getShadingSqDistance() const;
		uint32_t getTextureAnisotropy() const;

		uint32_t getPhysicsThrottling() const;

		float getLightGridRadius() const;
		bool getLightingNonFixedEnabled() const;
		unsigned getLightingChunkBudget() const;

		void SubmitCurrentFrame(double frameTime, double renderTime, double prepareTime, double bonusTime);

		// adjusts quality to try to fit rendering to this timespan.
		void ThrottleTo(double rendertime_ms);

		double getMetricValue(const std::string& metric);

		uint32_t GetRecomputeDistanceDelay() { return mRecomputeDistanceDelay; }

		float GetViewCullSqDistance();
		float GetRenderCullSqDistance();

		double GetMaxNextViewCullDistance(); // farthest cull distance possible in next frame.

		uint32_t GetQualityLevel() { return mCurrentQualityLevel; }

		bool IsBlockCullingEnabled() { return mBlockCullingEnabled; };
		void SetBlockCullingEnabled(bool enabled) { mBlockCullingEnabled = enabled; };

		// supply the framerate manager with some special information that can be
		// used to formulate exceptions.
		void Configure(const RenderCaps* renderCaps, CRenderSettings* settings);

		// after calling Configure, this gives our determination of the best
		// possible quality we can acheive with certain features, and with current settings
		CRenderSettings::AntialiasingMode getAntialiasingMode();
		void updateMaxSettings();

		double GetVisibleBlockTarget() const { return mBlockTarget; }; // smoothed block target
		double GetVisibleBlockCounter() const { return mLastBlockCounter; };

		float GetTargetFrameTimeForNextLevel() const;
		float GetTargetRenderTimeForNextLevel() const;

		// counter that indicates how many frames have elapsed with the block count in a stable state.
		void ResetStableFramesCounter() { mStableFramesCounter = 0u; };
		const uint32_t& GetStableFramesCounter() { return mStableFramesCounter; };

		// returns overall particle throttle factor. Range ]0 .. 1] , 1 for full detail.
		double GetParticleThrottleFactor();

		double GetRenderTimeAverage();
		double GetPrepareTimeAverage();
		double GetFrameTimeAverage();

		const WindowAverage<double, double>& GetRenderTimeStats();
		const WindowAverage<double, double>& GetFrameTimeStats();

		void StartCapturingMetrics();
		Metrics GetMetrics();

		void PauseAutoAdjustment();
		void ResumeAutoAdjustment();

		uint32_t GetQualityDelayUp() const { return mQualityDelayUp; }
		uint32_t GetQualityDelayDown() const { return mQualityDelayDown; }
		uint32_t GetBackoffCounter() const { return mBadBackoffFrameCounter; }
		double GetBackoffAverage() const { return fastBackoffAverage.getStats().average; }

	protected:
		bool mSSAOSupported;

		bool mAdjustmentOn;

		CRenderSettings* mSettings;
		const RenderCaps* mRenderCaps;

		bool mBlockCullingEnabled;
		bool mAggressivePerformance;

		uint32_t mStableFramesCounter;

		bool mThrottlingOn;

		uint32_t mCurrentQualityLevel;
		uint32_t mQualityCount[CRenderSettings::QualityLevelMax];

		uint32_t mQualityDelayUp;
		uint32_t mQualityDelayDown;
		uint32_t mRecomputeDistanceDelay;

		bool mWasQualityUp;
		uint32_t  mSwitchCounter;

	private:
		float mSqDistance;
		float mSqRenderDistance;

		void UpdateStats(double frameTime, double renderTime, double prepareTime);
		void AdjustQuality(double frameTime, double renderTime, bool adjustmentOn, double bonusTime);
		void StepQuality(bool direction, bool isBackOff);
		void UpdateQualitySettings();
		void SendQualityLevelStats();
		float GetAvarageQuality();

		float GetTargetFrameTime(uint32_t level) const;

		RBX::WindowAverage<double, double> frameTimeAverage;
		RBX::WindowAverage<double, double> renderTimeAverage;
		RBX::WindowAverage<double, double> prepareTimeAverage;
		RBX::WindowAverage<double, double> frameTimeVarianceAverage;

		RBX::WindowAverage<double, double> fastBackoffAverage;

		uint32_t mBadBackoffFrameCounter;

		Metrics mMetrics;
		RBX::Timer<RBX::Time::Fast> mSettleTimer;
		bool mIsStable;
		bool mIsGatheringDistance;
		uint32_t mBlockCounter;
		uint32_t mBlockTarget;
		uint32_t mLastBlockCounter;

		class  AvgFpsCounter {
		public:
			AvgFpsCounter() : timeSumSec(0.0), frameCnt(0u) {}

			void Update(double deltaTimeMs) {
				if (deltaTimeMs < 1000.0) {
					timeSumSec += deltaTimeMs * 0.001;
					++frameCnt;
				}
			}

			double GetFPS() { return frameCnt ? 1.0f / (timeSumSec / frameCnt) : 0u; }
		private:
			double timeSumSec;
			uint32_t frameCnt;
		};

		AvgFpsCounter mFPSCounter;

		THROTTLE_LOCKSTEP* LockstepTable;
	};

} // namespaces
