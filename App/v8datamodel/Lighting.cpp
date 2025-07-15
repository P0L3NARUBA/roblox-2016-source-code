#include "stdafx.h"

#include "v8datamodel/lighting.h"
#include "v8datamodel/sky.h"

using namespace RBX;

static Reflection::EventDesc<Lighting, void(bool)> event_LightingChanged(&Lighting::lightingChangedSignal, "LightingChanged", "skyboxChanged");
static Reflection::PropDescriptor<Lighting, std::string> prop_Time("TimeOfDay", category_Data, &Lighting::getTimeStr, &Lighting::setTimeStr);
static Reflection::PropDescriptor<Lighting, float> prop_ClockTime("ClockTime", category_Data, &Lighting::getTimeFloat, &Lighting::setTimeFloat);

static Reflection::PropDescriptor<Lighting, float> prop_GeographicLatitude("GeographicLatitude", category_Data, &Lighting::getGeographicLatitude, &Lighting::setGeographicLatitude);
static Reflection::BoundFuncDesc<Lighting, float()> prop_GetMoonPhase(&Lighting::getMoonPhase, "GetMoonPhase", Security::None);
static Reflection::BoundFuncDesc<Lighting, G3D::Vector3()> prop_GetMoonPosition(&Lighting::getMoonPosition, "GetMoonDirection", Security::None);
static Reflection::BoundFuncDesc<Lighting, G3D::Vector3()> prop_GetSunPosition(&Lighting::getSunPosition, "GetSunDirection", Security::None);

static Reflection::BoundFuncDesc<Lighting, double()> func_GetMinutesAfterMidnight(&Lighting::getMinutesAfterMidnight, "GetMinutesAfterMidnight", Security::None);
static Reflection::BoundFuncDesc<Lighting, void(double)> func_SetMinutesAfterMidnight(&Lighting::setMinutesAfterMidnight, "SetMinutesAfterMidnight", "minutes", Security::None);
static Reflection::BoundFuncDesc<Lighting, double()> func_dep_GetMinutesAfterMidnight(&Lighting::getMinutesAfterMidnight, "getMinutesAfterMidnight", Security::None, Reflection::Descriptor::Attributes::deprecated(func_GetMinutesAfterMidnight));
static Reflection::BoundFuncDesc<Lighting, void(double)> func_dep_SetMinutesAfterMidnight(&Lighting::setMinutesAfterMidnight, "setMinutesAfterMidnight", "minutes", Security::None, Reflection::Descriptor::Attributes::deprecated(func_SetMinutesAfterMidnight));

/* Sun */
Reflection::PropDescriptor<Lighting, float>		  Lighting::prop_SunBrightness("Brightness", "Sun", &Lighting::getSunBrightness, &Lighting::setSunBrightness);
Reflection::PropDescriptor<Lighting, G3D::Color3> Lighting::prop_SunColorShift("Color Shift", "Sun", &Lighting::getSunColorShift, &Lighting::setSunColorShift);
Reflection::PropDescriptor<Lighting, bool>		  Lighting::prop_SunShadows("Cast Shadows", "Sun", &Lighting::getSunShadows, &Lighting::setSunShadows);

/* Environment */
Reflection::PropDescriptor<Lighting, G3D::Color3> Lighting::prop_GlobalAmbient("Ambient", "Environment", &Lighting::getGlobalAmbient, &Lighting::setGlobalAmbient);
Reflection::PropDescriptor<Lighting, G3D::Color3> Lighting::prop_GlobalOutdoorAmbient("Outdoor Ambient", "Environment", &Lighting::getGlobalOutdoorAmbient, &Lighting::setGlobalOutdoorAmbient);
Reflection::PropDescriptor<Lighting, float>		  Lighting::prop_GlobalDiffuseFactor("Env. Diffuse Factor", "Environment", &Lighting::getGlobalDiffuseFactor, &Lighting::setGlobalDiffuseFactor);
Reflection::PropDescriptor<Lighting, float>		  Lighting::prop_GlobalSpecularFactor("Env. Specular Factor", "Environment", &Lighting::getGlobalSpecularFactor, &Lighting::setGlobalSpecularFactor);
Reflection::PropDescriptor<Lighting, bool>		  Lighting::prop_GlobalShadows("Global Shadows", "Environment", &Lighting::getGlobalShadows, &Lighting::setGlobalShadows);
Reflection::PropDescriptor<Lighting, bool>		  Lighting::prop_GlobalSSAO("SSAO", "Environment", &Lighting::getGlobalSSAO, &Lighting::setGlobalSSAO);

/* Fog */
Reflection::PropDescriptor<Lighting, bool>		  Lighting::prop_FogAffectSkybox("Affect Skybox", "Fog", &Lighting::getFogAffectSkybox, &Lighting::setFogAffectSkybox);
Reflection::PropDescriptor<Lighting, G3D::Color3> Lighting::prop_FogColor("Color", "Fog", &Lighting::getFogColor, &Lighting::setFogColor);
Reflection::PropDescriptor<Lighting, float>		  Lighting::prop_FogDensity("Density", "Fog", &Lighting::getFogDensity, &Lighting::setFogDensity);
Reflection::PropDescriptor<Lighting, bool>		  Lighting::prop_FogSkybox("Use Skybox", "Fog", &Lighting::getFogSkybox, &Lighting::setFogSkybox);
Reflection::PropDescriptor<Lighting, float>		  Lighting::prop_FogSunInfluence("Sun Influence", "Fog", &Lighting::getFogSunInfluence, &Lighting::setFogSunInfluence);

/* Camera */
Reflection::PropDescriptor<Lighting, bool>		  Lighting::prop_CameraAutoExposure("Automatic", "Camera", &Lighting::getCameraAutoExposure, &Lighting::setCameraAutoExposure);
Reflection::PropDescriptor<Lighting, float>		  Lighting::prop_CameraExposure("Exposure", "Camera", &Lighting::getCameraExposure, &Lighting::setCameraExposure);

/* Deprecated */
Reflection::PropDescriptor<Lighting, G3D::Color3> Lighting::prop_TopColorShift_deprecated("ColorShift_Top", category_Appearance, &Lighting::getTopColorShift_deprecated, &Lighting::setTopColorShift_deprecated, Reflection::PropertyDescriptor::Attributes::deprecated(prop_SunColorShift, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING));
Reflection::PropDescriptor<Lighting, G3D::Color3> Lighting::prop_BottomColorShift_deprecated("ColorShift_Bottom", category_Appearance, &Lighting::getBottomColorShift_deprecated, &Lighting::setBottomColorShift_deprecated, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING);
Reflection::PropDescriptor<Lighting, bool>		  Lighting::prop_GlobalShadows_deprecated("GlobalShadows", category_Appearance, &Lighting::getGlobalShadows_deprecated, &Lighting::setGlobalShadows_deprecated, Reflection::PropertyDescriptor::Attributes::deprecated(prop_GlobalShadows, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING));
Reflection::PropDescriptor<Lighting, G3D::Color3> Lighting::prop_GlobalOutdoorAmbient_deprecated("OutdoorAmbient", category_Appearance, &Lighting::getGlobalOutdoorAmbient_deprecated, &Lighting::setGlobalOutdoorAmbient_deprecated, Reflection::PropertyDescriptor::Attributes::deprecated(prop_GlobalOutdoorAmbient, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING));
Reflection::PropDescriptor<Lighting, bool>		  Lighting::prop_Outlines_deprecated("Outlines", category_Appearance, &Lighting::getOutlines_deprecated, &Lighting::setOutlines_deprecated, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING);
Reflection::PropDescriptor<Lighting, G3D::Color3> Lighting::prop_ShadowColor_deprecated("ShadowColor", category_Appearance, &Lighting::getShadowColor_deprecated, &Lighting::setShadowColor_deprecated, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING);

Reflection::PropDescriptor<Lighting, G3D::Color3> Lighting::prop_FogColor_deprecated("FogColor", "Fog", &Lighting::getFogColor_deprecated, &Lighting::setFogColor_deprecated, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING);
Reflection::PropDescriptor<Lighting, float>		  Lighting::prop_FogStart_deprecated("FogStart", "Fog", &Lighting::getFogStart_deprecated, &Lighting::setFogStart_deprecated, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING);
Reflection::PropDescriptor<Lighting, float>		  Lighting::prop_FogEnd_deprecated("FogEnd", "Fog", &Lighting::getFogEnd_deprecated, &Lighting::setFogEnd_deprecated, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING);

/*
Reflection::BoundProp<float> Lighting::desc_GlobalBrightness("Brightness", category_Appearance, &Lighting::globalBrightness, &Lighting::onPropChanged);
Reflection::BoundProp<G3D::Color3> Lighting::desc_TopColorShift("Sun Color", category_Appearance, &Lighting::topColorShift, &Lighting::onPropChanged);
Reflection::BoundProp<G3D::Color3> Lighting::desc_BottomColorShift("ColorShift_Bottom", category_Appearance, &Lighting::bottomColorShift, &Lighting::onPropChanged);
Reflection::BoundProp<G3D::Color3> Lighting::desc_GlobalAmbient("Ambient", category_Appearance, &Lighting::globalAmbient, &Lighting::onPropChanged);
Reflection::BoundProp<G3D::Color3> Lighting::desc_GlobalOutdoorAmbient("OutdoorAmbient", category_Appearance, &Lighting::globalOutdoorAmbient, &Lighting::onPropChanged);
Reflection::BoundProp<bool> Lighting::desc_Outlines("Outlines", category_Appearance, &Lighting::outlines, &Lighting::onPropChanged);
*/

const char* const RBX::sLighting = "Lighting";

static G3D::Color3 fromRGB(unsigned char r, unsigned char g, unsigned char b) {
	return G3D::Color3(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
}

Lighting::Lighting(void)
	:clearColor(G3D::Color3::black())
	, clearAlpha(1.0f)
	, hasSky(true)

	, timeOfDay(boost::posix_time::duration_from_string("14:00:00"))

	, sunBrightness(1.0f)
	, sunColorShift(G3D::Color3(1.0f, 1.0f, 1.0f))
	, sunShadows(true)

	, globalAmbient(G3D::Color3(0.0f, 0.0f, 0.0f))
	, globalOutdoorAmbient(G3D::Color3(0.0f, 0.0f, 0.0f))
	, globalDiffuseFactor(1.0f)
	, globalSpecularFactor(1.0f)
	, globalShadows(true)

	, fogAffectSkybox(false)
	, fogColor(G3D::Color3(0.0f, 0.0f, 0.0f))
	, fogDensity(1.0f)
	, fogSkybox(false)
	, fogSunInfluence(1.0f)

	, cameraAutoExposure(false)
	, cameraExposure(0.0f)

	, topColorShift_deprecated(G3D::Color3(0.0f, 0.0f, 0.0f))
	, bottomColorShift_deprecated(G3D::Color3(0.0f, 0.0f, 0.0f))
	, globalShadows_deprecated(true)
	, globalOutdoorAmbient_deprecated(G3D::Color3(0.0f, 0.0f, 0.0f))
	, outlines_deprecated(false)
	, shadowColor_deprecated(G3D::Color3(0.0f, 0.0f, 0.0f))

	, fogColor_deprecated(G3D::Color3(0.0f, 0.0f, 0.0f))
	, fogEnd_deprecated(10000.0f)
	, fogStart_deprecated(0.0f)
{
	skyParameters.lightColor = fromRGB(152, 137, 102);
	skyParameters.setTime(getGameTime());
	setName("Lighting");
}

G3D::Vector3 Lighting::getMoonPosition() {
	return skyParameters.physicallyCorrect ? skyParameters.trueMoonPosition : skyParameters.moonPosition;
}

G3D::Vector3 Lighting::getSunPosition() {
	return skyParameters.physicallyCorrect ? skyParameters.trueSunPosition : skyParameters.sunPosition;
}

double Lighting::getMinutesAfterMidnight() {
	return timeOfDay.total_milliseconds() / 60000.0;
}

void Lighting::setMinutesAfterMidnight(double value) {
	setTime(boost::posix_time::seconds((int)(60 * value)));
}

G3D::GameTime Lighting::getGameTime() const {
	return timeOfDay.total_seconds();
}

std::string Lighting::getTimeStr() const {
	return boost::posix_time::to_simple_string(timeOfDay);
}

void Lighting::setTimeStr(const std::string& value) {
	setTime(boost::posix_time::duration_from_string(value));
}

float Lighting::getTimeFloat() const {
	return (timeOfDay.total_seconds() / 3600.0);
}

void Lighting::setTimeFloat(float value) {
	value = G3D::clamp(value, 0.0f, 24.0f);
	int seconds = ceil(value * 3600);

	if (timeOfDay.total_seconds() != seconds)
	{
		timeOfDay = boost::posix_time::seconds(seconds);

		skyParameters.setTime(getGameTime());
		this->raisePropertyChanged(prop_ClockTime);
		fireLightingChanged(false);
	}
}

void Lighting::setTime(const boost::posix_time::time_duration& value) {
	// wrap to 24-hour clock
	int seconds = value.total_seconds();
	seconds %= 60 * 60 * 24;

	if (timeOfDay.total_seconds() != seconds)
	{
		timeOfDay = boost::posix_time::seconds(seconds);

		skyParameters.setTime(getGameTime());
		this->raisePropertyChanged(prop_Time);
		fireLightingChanged(false);
	}
}

void Lighting::setGeographicLatitude(float value) {
	if (value != skyParameters.geoLatitude)
	{
		skyParameters.setLatitude(value);
		skyParameters.setTime(getGameTime());

		this->raisePropertyChanged(prop_GeographicLatitude);
		fireLightingChanged(false);
	}
}

bool Lighting::askAddChild(const Instance* instance) const {
	return Instance::fastDynamicCast<Sky>(instance) != 0;
}

void Lighting::fireLightingChanged(bool skyboxChanged) {
	lightingChangedSignal(skyboxChanged);
}

void Lighting::setClearColor(const Color3& value) {
	if (value != clearColor)
	{
		clearColor = value;
		fireLightingChanged(false);
	}
}

void Lighting::setClearAlpha(float value) {
	if (value != clearAlpha)
	{
		clearAlpha = value;
		fireLightingChanged(false);
	}
}

/* Sun */
void Lighting::setSunBrightness(float value) {
	value = std::max(value, 0.0f);

	if (sunBrightness != value)
	{
		sunBrightness = value;
		this->raisePropertyChanged(prop_SunBrightness);
		fireLightingChanged(false);
	}
}

void Lighting::setSunColorShift(const Color3& value) {
	if (sunColorShift != value)
	{
		sunColorShift = value;
		this->raisePropertyChanged(prop_SunColorShift);
		fireLightingChanged(false);
	}
}

void Lighting::setSunShadows(bool value) {
	if (sunShadows != value)
	{
		sunShadows = value;
		this->raisePropertyChanged(prop_SunShadows);
	}
}

/* Environment */
void Lighting::setGlobalAmbient(const Color3& value) {
	if (globalAmbient != value)
	{
		globalAmbient = value;
		this->raisePropertyChanged(prop_GlobalAmbient);
		fireLightingChanged(false);
	}
}

void Lighting::setGlobalOutdoorAmbient(const Color3& value) {
	if (globalOutdoorAmbient != value)
	{
		globalOutdoorAmbient = value;
		this->raisePropertyChanged(prop_GlobalOutdoorAmbient);
		fireLightingChanged(false);
	}
}

void Lighting::setGlobalDiffuseFactor(float value) {
	value = std::min(std::max(value, 0.0f), 1.0f);

	if (globalDiffuseFactor != value)
	{
		globalDiffuseFactor = value;
		this->raisePropertyChanged(prop_GlobalDiffuseFactor);
	}
}

void Lighting::setGlobalSpecularFactor(float value) {
	value = std::min(std::max(value, 0.0f), 1.0f);

	if (globalSpecularFactor != value)
	{
		globalSpecularFactor = value;
		this->raisePropertyChanged(prop_GlobalSpecularFactor);
	}
}

void Lighting::setGlobalShadows(bool value) {
	if (globalShadows != value)
	{
		globalShadows = value;
		this->raisePropertyChanged(prop_GlobalShadows);
	}
}

void Lighting::setGlobalSSAO(bool value) {
	if (globalSSAO != value)
	{
		globalSSAO = value;
		this->raisePropertyChanged(prop_GlobalSSAO);
	}
}

/* Fog */
void Lighting::setFogAffectSkybox(bool value) {
	if (fogAffectSkybox != value)
	{
		fogAffectSkybox = value;
		this->raisePropertyChanged(prop_FogAffectSkybox);
	}
}

void Lighting::setFogColor(const Color3& value) {
	if (fogColor != value)
	{
		fogColor = value;
		this->raisePropertyChanged(prop_FogColor);
	}
}

void Lighting::setFogDensity(float value) {
	value = std::max(value, 0.0f);

	if (fogDensity != value)
	{
		fogDensity = value;
		this->raisePropertyChanged(prop_FogDensity);
	}
}

void Lighting::setFogSkybox(bool value) {
	if (fogSkybox != value)
	{
		fogSkybox = value;
		this->raisePropertyChanged(prop_FogSkybox);
	}
}

void Lighting::setFogSunInfluence(float value) {
	value = std::max(value, 0.0f);

	if (fogSunInfluence != value)
	{
		fogSunInfluence = value;
		this->raisePropertyChanged(prop_FogSunInfluence);
	}
}

/* Camera */
void Lighting::setCameraAutoExposure(bool value) {
	if (cameraAutoExposure != value)
	{
		cameraAutoExposure = value;
		this->raisePropertyChanged(prop_CameraAutoExposure);
	}
}

void Lighting::setCameraExposure(float value) {
	if (cameraExposure != value)
	{
		cameraExposure = value;
		this->raisePropertyChanged(prop_CameraExposure);
	}
}

/* Deprecated */
void Lighting::setTopColorShift_deprecated(const Color3& value) {
	if (topColorShift_deprecated != value)
	{
		topColorShift_deprecated = value;
		this->raisePropertyChanged(prop_TopColorShift_deprecated);
		fireLightingChanged(false);
	}
}

void Lighting::setBottomColorShift_deprecated(const Color3& value) {
	if (bottomColorShift_deprecated != value)
	{
		bottomColorShift_deprecated = value;
		this->raisePropertyChanged(prop_BottomColorShift_deprecated);
		fireLightingChanged(false);
	}
}

void Lighting::setGlobalShadows_deprecated(bool value) {
	if (globalShadows_deprecated != value)
	{
		globalShadows_deprecated = value;
		this->raisePropertyChanged(prop_GlobalOutdoorAmbient_deprecated);
	}
}

void Lighting::setGlobalOutdoorAmbient_deprecated(const Color3& value) {
	if (globalOutdoorAmbient_deprecated != value)
	{
		globalOutdoorAmbient_deprecated = value;
		this->raisePropertyChanged(prop_GlobalShadows_deprecated);
		fireLightingChanged(false);
	}
}

void Lighting::setOutlines_deprecated(bool value) {
	if (outlines_deprecated != value)
	{
		outlines_deprecated = value;
		this->raisePropertyChanged(prop_Outlines_deprecated);
	}
}

void Lighting::setShadowColor_deprecated(const Color3& value) {
	if (shadowColor_deprecated != value)
	{
		shadowColor_deprecated = value;
		this->raisePropertyChanged(prop_ShadowColor_deprecated);
		fireLightingChanged(false);
	}
}

void Lighting::setFogColor_deprecated(const Color3& value) {
	if (fogColor_deprecated != value)
	{
		fogColor_deprecated = value;
		this->raisePropertyChanged(prop_FogColor_deprecated);
	}
}

void Lighting::setFogEnd_deprecated(float value) {
	value = std::max(value, 0.0f);

	if (fogEnd_deprecated != value)
	{
		fogEnd_deprecated = value;
		this->raisePropertyChanged(prop_FogStart_deprecated);
	}
}

void Lighting::setFogStart_deprecated(float value) {
	value = std::max(value, 0.0f);

	if (fogStart_deprecated != value)
	{
		fogStart_deprecated = value;
		this->raisePropertyChanged(prop_FogEnd_deprecated);
	}
}

void Lighting::replaceSky(Sky* newSky) {
	while (Sky* oldSky = this->findFirstChildOfType<Sky>())
	{
		oldSky->setParent(NULL);
	}
	newSky->setParent(this);
}


void Lighting::onChildRemoving(Instance* child) {
	if (sky.get() == child)
	{
		sky.reset();
		fireLightingChanged(true);
	}
	Super::onChildRemoving(child);
}

void Lighting::onChildAdded(Instance* child) {
	Super::onChildAdded(child);
	if (Sky* sky = Instance::fastDynamicCast<Sky>(child))
	{
		this->sky = shared_from(sky);
		fireLightingChanged(true);
	}
}

void Lighting::onChildChanged(Instance* instance, const PropertyChanged& event) {
	Super::onChildChanged(instance, event);
	if (sky.get() == instance)
		fireLightingChanged(true);
}
