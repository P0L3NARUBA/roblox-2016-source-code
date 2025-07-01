#include "stdafx.h"

#include "V8DataModel/Light.h"

#include "V8DataModel/PartInstance.h"

#include "Util/RobloxGoogleAnalytics.h"

static void sendLightingObjectsStats()
{
	RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "LightingObjects");
}

namespace RBX {

const char* const sLight = "Light";
const char* const sPointLight = "PointLight";
const char* const sSpotLight = "SpotLight";
const char* const sAreaLight = "AreaLight";

/* Base Light */
Reflection::PropDescriptor<Light, bool> Light::prop_Enabled("Enabled", category_Appearance, &Light::getEnabled, &Light::setEnabled);
Reflection::PropDescriptor<Light, Color3> Light::prop_Color("Color", category_Appearance, &Light::getColor, &Light::setColor);
Reflection::PropDescriptor<Light, float> Light::prop_Brightness("Brightness", category_Appearance, &Light::getBrightness, &Light::setBrightness);
Reflection::PropDescriptor<Light, float> Light::prop_Range("Range", category_Appearance, &Light::getRange, &Light::setRange);
Reflection::PropDescriptor<Light, float> Light::prop_Attenuation("Attenuation", category_Appearance, &Light::getAttenuation, &Light::setAttenuation);

Reflection::PropDescriptor<Light, bool> Light::prop_Shadows("Enabled", "Shadows", &Light::getShadows, &Light::setShadows);
Reflection::PropDescriptor<Light, bool> Light::prop_Colored("Colored", "Shadows", &Light::getColored, &Light::setColored);

/* Spot Light */
Reflection::PropDescriptor<SpotLight, float> SpotLight::prop_OuterAngle("Outer Angle", category_Appearance, &SpotLight::getOuterAngle, &SpotLight::setOuterAngle);
Reflection::PropDescriptor<SpotLight, float> SpotLight::prop_InnerAngle("Inner Angle", category_Appearance, &SpotLight::getInnerAngle, &SpotLight::setInnerAngle);
Reflection::EnumPropDescriptor<SpotLight, NormalId> SpotLight::prop_Face("Face", category_Appearance, &SpotLight::getFace, &SpotLight::setFace);

/* Area Light */
Reflection::PropDescriptor<AreaLight, float> AreaLight::prop_OuterAngle("Outer Angle", category_Appearance, &AreaLight::getOuterAngle, &AreaLight::setOuterAngle);
Reflection::PropDescriptor<AreaLight, float> AreaLight::prop_InnerAngle("Inner Angle", category_Appearance, &AreaLight::getInnerAngle, &AreaLight::setInnerAngle);
Reflection::EnumPropDescriptor<AreaLight, NormalId> AreaLight::prop_Face("Face", category_Appearance, &AreaLight::getFace, &AreaLight::setFace);

Light::Light(const char* name) 
	: DescribedNonCreatable<Light, Instance, sLight>(name)
	, enabled(true)
	, color(Color3::white())
	, brightness(1.0f)
	, range(16.0f)
	, attenuation(0.0f)
	, shadows(false)
	, colored(false)
{
    static boost::once_flag flag = BOOST_ONCE_INIT;
    boost::call_once(&sendLightingObjectsStats, flag);
}

Light::~Light()
{
}

void Light::setEnabled(bool value)
{
	if (enabled != value) {
		enabled = value;
		raisePropertyChanged(prop_Enabled);
	}
}

void Light::setColor(Color3 value)
{
	if (color != value) {
		color = value;
		raisePropertyChanged(prop_Color);
	}
}

void Light::setBrightness(float value)
{
	value = std::max(value, 0.f);
	
	if (brightness != value) {
		brightness = value;
		raisePropertyChanged(prop_Brightness);
	}
}

void Light::setRange(float value)
{
	value = std::min(std::max(value, 0.0f), 512.0f);

	if (range != value) {
		range = value;
		raisePropertyChanged(prop_Range);
	}
}

void Light::setAttenuation(float value)
{
	value = std::min(std::max(value, 0.0f), 1.0f);

	if (attenuation != value) {
		attenuation = value;
		raisePropertyChanged(prop_Attenuation);
	}
}

void Light::setShadows(bool value)
{
	if (shadows != value) {
		shadows = value;
		raisePropertyChanged(prop_Shadows);
	}
}

void Light::setColored(bool value)
{
	if (colored != value) {
		colored = value;
		raisePropertyChanged(prop_Colored);
	}
}

bool Light::askSetParent(const Instance* parent) const
{
	return Instance::isA<PartInstance>(parent);
}

bool Light::askAddChild(const Instance* instance) const
{
	return true;
}

PointLight::PointLight() 
	: DescribedCreatable<PointLight, Light, sPointLight>("PointLight")
{
}

PointLight::~PointLight()
{
}

SpotLight::SpotLight() 
	: DescribedCreatable<SpotLight, Light, sSpotLight>("SpotLight")
	, outerAngle(90.0f)
	, innerAngle(0.0f)
	, face(NORM_Z_NEG)
{
}

SpotLight::~SpotLight()
{
}
        
void SpotLight::setOuterAngle(float value)
{
	value = std::min(std::max(value, 0.0f), 180.0f);
	
	if (outerAngle != value) {
		outerAngle = value;
		raisePropertyChanged(prop_OuterAngle);
	}
	if (innerAngle > value) {
		innerAngle = value;
		raisePropertyChanged(prop_InnerAngle);
	}
}

void SpotLight::setInnerAngle(float value)
{
	value = std::min(std::max(value, 0.0f), 180.0f);

	if (innerAngle != value) {
		innerAngle = value;
		raisePropertyChanged(prop_InnerAngle);
	}
	if (outerAngle < value) {
		outerAngle = value;
		raisePropertyChanged(prop_OuterAngle);
	}
}

void SpotLight::setFace(NormalId value)
{
	if (face != value) {
		face = value;
        raisePropertyChanged(prop_Face);
	}
}

AreaLight::AreaLight() 
	: DescribedCreatable<AreaLight, Light, sAreaLight>("AreaLight")
	, outerAngle(90.0f)
	, innerAngle(0.0f)
	, face(NORM_Z_NEG)
{
}

AreaLight::~AreaLight()
{
}

void AreaLight::setOuterAngle(float value)
{
	value = std::min(std::max(value, 0.0f), 180.0f);

	if (outerAngle != value) {
		outerAngle = value;
		raisePropertyChanged(prop_OuterAngle);
	}
	if (innerAngle > value) {
		innerAngle = value;
		raisePropertyChanged(prop_InnerAngle);
	}
}

void AreaLight::setInnerAngle(float value)
{
	value = std::min(std::max(value, 0.0f), 180.0f);

	if (innerAngle != value) {
		innerAngle = value;
		raisePropertyChanged(prop_InnerAngle);
	}
	if (outerAngle < value) {
		outerAngle = value;
		raisePropertyChanged(prop_OuterAngle);
	}
}

void AreaLight::setFace(NormalId value)
{
	if (face != value) {
		face = value;
        raisePropertyChanged(prop_Face);
	}
}

} // namespace
