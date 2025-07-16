#pragma once

#include "V8Tree/Service.h"
#include "util/g3dcore.h"
#include "G3D/LightingParameters.h"
/*#define BOOST_DATE_TIME_NO_LIB
#include "boost/date_time/posix_time/posix_time.hpp"*/

namespace RBX {

	class Sky;

	extern const char* const sLighting;
	class Lighting
		: public DescribedCreatable<Lighting, Instance, sLighting, Reflection::ClassDescriptor::PERSISTENT_HIDDEN>
		, public Service
	{
	public:
		Lighting();

		void replaceSky(Sky* newSky);

		rbx::signal<void(bool)> lightingChangedSignal;		// (arg=skyboxChanged)

		bool isSkySuppressed() const { return !hasSky; }
		void suppressSky(const bool value) { hasSky = !value; }

		shared_ptr<Sky> sky;

		/* Sun */
		static Reflection::PropDescriptor<Lighting, float>		 prop_SunBrightness;
		static Reflection::PropDescriptor<Lighting, G3D::Color3> prop_SunColorShift;
		static Reflection::PropDescriptor<Lighting, bool>		 prop_SunShadows;

		/* Environment */
		static Reflection::PropDescriptor<Lighting, G3D::Color3> prop_GlobalAmbient;
		static Reflection::PropDescriptor<Lighting, G3D::Color3> prop_GlobalOutdoorAmbient;
		static Reflection::PropDescriptor<Lighting, float>		 prop_GlobalDiffuseFactor;
		static Reflection::PropDescriptor<Lighting, float>		 prop_GlobalSpecularFactor;
		static Reflection::PropDescriptor<Lighting, bool>		 prop_GlobalShadows;
		static Reflection::PropDescriptor<Lighting, bool>		 prop_GlobalSSAO;

		/* Fog */
		static Reflection::PropDescriptor<Lighting, bool>		 prop_FogAffectSkybox;
		static Reflection::PropDescriptor<Lighting, G3D::Color3> prop_FogColor;
		static Reflection::PropDescriptor<Lighting, float>		 prop_FogDensity;
		static Reflection::PropDescriptor<Lighting, bool>		 prop_FogSkybox;
		static Reflection::PropDescriptor<Lighting, float>		 prop_FogSunInfluence;

		/* Camera */
		static Reflection::PropDescriptor<Lighting, bool>		 prop_CameraAutoExposure;
		static Reflection::PropDescriptor<Lighting, float>		 prop_CameraExposure;

		/* Deprecated */
		static Reflection::PropDescriptor<Lighting, G3D::Color3> prop_TopColorShift_deprecated;
		static Reflection::PropDescriptor<Lighting, G3D::Color3> prop_BottomColorShift_deprecated;
		static Reflection::PropDescriptor<Lighting, G3D::Color3> prop_GlobalOutdoorAmbient_deprecated;
		static Reflection::PropDescriptor<Lighting, bool>		 prop_GlobalShadows_deprecated;
		static Reflection::PropDescriptor<Lighting, bool>		 prop_Outlines_deprecated;
		static Reflection::PropDescriptor<Lighting, G3D::Color3> prop_ShadowColor_deprecated;

		static Reflection::PropDescriptor<Lighting, G3D::Color3> prop_FogColor_deprecated;
		static Reflection::PropDescriptor<Lighting, float>		 prop_FogStart_deprecated;
		static Reflection::PropDescriptor<Lighting, float>		 prop_FogEnd_deprecated;

		const G3D::LightingParameters& getSkyParameters() const { return skyParameters; }

		const G3D::Color3& getClearColor() const { return clearColor; }
		void setClearColor(const G3D::Color3& value);

		float getClearAlpha() const { return clearAlpha; }
		void setClearAlpha(float value);

		/* Time */
		double getMinutesAfterMidnight();
		void setMinutesAfterMidnight(double value);

		float getTimeFloat() const;
		void setTimeFloat(const float value);

		std::string getTimeStr() const;
		void setTimeStr(const std::string& value);

		void setTime(const boost::posix_time::time_duration& value);
		G3D::GameTime getGameTime() const;

		float getMoonPhase() { return (float)skyParameters.moonPhase; }
		G3D::Vector3 getMoonPosition();
		G3D::Vector3 getSunPosition();

		float getGeographicLatitude() const { return skyParameters.geoLatitude; }
		void setGeographicLatitude(float value);

		/* Sun */
		float getSunBrightness() const { return sunBrightness; }
		void setSunBrightness(float value);

		const G3D::Color3& getSunColorShift() const { return sunColorShift; }
		void setSunColorShift(const G3D::Color3& value);

		bool getSunShadows() const { return sunShadows; }
		void setSunShadows(bool value);

		/* Environment */
		const G3D::Color3& getGlobalAmbient() const { return globalAmbient; }
		void setGlobalAmbient(const G3D::Color3& value);

		const G3D::Color3& getGlobalOutdoorAmbient() const { return globalOutdoorAmbient; }
		void setGlobalOutdoorAmbient(const G3D::Color3& value);

		float getGlobalDiffuseFactor() const { return globalDiffuseFactor; }
		void setGlobalDiffuseFactor(float value);

		float getGlobalSpecularFactor() const { return globalSpecularFactor; }
		void setGlobalSpecularFactor(float value);

		bool getGlobalShadows() const { return globalShadows; }
		void setGlobalShadows(bool value);

		bool getGlobalSSAO() const { return globalSSAO; }
		void setGlobalSSAO(bool value);

		/* Fog */
		bool getFogAffectSkybox() const { return fogAffectSkybox; }
		void setFogAffectSkybox(bool value);

		const G3D::Color3& getFogColor() const { return fogColor; }
		void setFogColor(const G3D::Color3& value);

		float getFogDensity() const { return fogDensity; }
		void setFogDensity(float value);

		bool getFogSkybox() const { return fogSkybox; }
		void setFogSkybox(bool value);

		float getFogSunInfluence() const { return fogSunInfluence; }
		void setFogSunInfluence(float value);

		/* Camera */
		bool getCameraAutoExposure() const { return cameraAutoExposure; }
		void setCameraAutoExposure(bool value);

		float getCameraExposure() const { return cameraExposure; }
		void setCameraExposure(float value);

		/* Deprecated */
		const G3D::Color3& getTopColorShift_deprecated() const { return topColorShift_deprecated; }
		void setTopColorShift_deprecated(const G3D::Color3& value);

		const G3D::Color3& getBottomColorShift_deprecated() const { return bottomColorShift_deprecated; }
		void setBottomColorShift_deprecated(const G3D::Color3& value);

		bool getGlobalShadows_deprecated() const { return globalShadows_deprecated; }
		void setGlobalShadows_deprecated(bool value);

		const G3D::Color3& getGlobalOutdoorAmbient_deprecated() const { return globalOutdoorAmbient_deprecated; }
		void setGlobalOutdoorAmbient_deprecated(const G3D::Color3& value);

		bool getOutlines_deprecated() const { return outlines_deprecated; }
		void setOutlines_deprecated(bool value);

		const G3D::Color3& getShadowColor_deprecated() const { return shadowColor_deprecated; }
		void setShadowColor_deprecated(const G3D::Color3& value);

		const G3D::Color3& getFogColor_deprecated() const { return fogColor_deprecated; }
		void setFogColor_deprecated(const G3D::Color3& value);

		float getFogEnd_deprecated() const { return fogEnd_deprecated; }
		void setFogEnd_deprecated(float value);

		float getFogStart_deprecated() const { return fogStart_deprecated; }
		void setFogStart_deprecated(float value);

	protected:
		/*override*/ void onChildAdded(Instance* child);
		/*override*/ void onChildRemoving(Instance* child);
		/*override*/ void onChildChanged(Instance* instance, const PropertyChanged& event);
		/*override*/ bool askAddChild(const Instance* instance) const;
	private:
		void onPropChanged(const Reflection::PropertyDescriptor&)
		{
			fireLightingChanged(false);
		}
		void fireLightingChanged(bool skyboxChanged);

		typedef DescribedCreatable<Lighting, Instance, sLighting, Reflection::ClassDescriptor::PERSISTENT_HIDDEN> Super;
		G3D::LightingParameters skyParameters;
		bool hasSky;
		G3D::Color3 clearColor;
		float clearAlpha;

		/* Time */
		boost::posix_time::time_duration timeOfDay;

		/* Sun */
		float sunBrightness;
		Color3 sunColorShift;
		bool sunShadows;

		/* Environment */
		Color3 globalAmbient;
		Color3 globalOutdoorAmbient;
		float globalDiffuseFactor;
		float globalSpecularFactor;
		bool globalShadows;
		bool globalSSAO;

		/* Fog */
		bool fogAffectSkybox;
		Color3 fogColor;
		float fogDensity;
		bool fogSkybox;
		float fogSunInfluence;

		/* Camera */
		bool cameraAutoExposure;
		float cameraExposure;

		/* Deprecated */
		Color3 topColorShift_deprecated;
		Color3 bottomColorShift_deprecated;
		bool globalShadows_deprecated;
		Color3 globalOutdoorAmbient_deprecated;
		bool outlines_deprecated;
		Color3 shadowColor_deprecated;

		Color3 fogColor_deprecated;
		float fogEnd_deprecated;
		float fogStart_deprecated;

	};

}
