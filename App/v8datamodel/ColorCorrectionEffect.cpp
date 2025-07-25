#include "stdafx.h"
#include "V8DataModel/ColorCorrectionEffect.h"

namespace RBX {

	const char* const sColorCorrectionEffect = "ColorCorrectionEffect";

	Reflection::PropDescriptor<ColorCorrectionEffect, float> ColorCorrectionEffect::prop_Brightness("Brightness", category_Appearance, &ColorCorrectionEffect::getBrightness, &ColorCorrectionEffect::setBrightness);
	Reflection::PropDescriptor<ColorCorrectionEffect, float> ColorCorrectionEffect::prop_Contrast("Contrast", category_Appearance, &ColorCorrectionEffect::getContrast, &ColorCorrectionEffect::setContrast);
	Reflection::PropDescriptor<ColorCorrectionEffect, float> ColorCorrectionEffect::prop_Saturation("Saturation", category_Appearance, &ColorCorrectionEffect::getSaturation, &ColorCorrectionEffect::setSaturation);
	Reflection::PropDescriptor<ColorCorrectionEffect, Color3> ColorCorrectionEffect::prop_TintColor("Tint Color", category_Appearance, &ColorCorrectionEffect::getTintColor, &ColorCorrectionEffect::setTintColor);

	Reflection::PropDescriptor<ColorCorrectionEffect, Color3> ColorCorrectionEffect::prop_TintColor_deprecated("TintColor", category_Appearance, &ColorCorrectionEffect::getTintColor_deprecated, &ColorCorrectionEffect::setTintColor_deprecated, Reflection::PropertyDescriptor::Attributes::deprecated(prop_TintColor, Reflection::PropertyDescriptor::LEGACY_SCRIPTING));
	
	ColorCorrectionEffect::ColorCorrectionEffect()
		:DescribedCreatable<ColorCorrectionEffect, PostEffect, sColorCorrectionEffect>("ColorCorrectionEffect")
		, brightness(1.0f)
		, contrast(1.0f)
		, saturation(1.0f)
		, tintColor(Color3(1.0f, 1.0f, 1.0f))
	{
	}

	ColorCorrectionEffect::~ColorCorrectionEffect()
	{
	}

	void ColorCorrectionEffect::setBrightness(float value)
	{
		if (brightness != value) {
			brightness = value;
			raisePropertyChanged(prop_Brightness);
		}
	}

	void ColorCorrectionEffect::setContrast(float value)
	{
		if (contrast != value) {
			contrast = value;
			raisePropertyChanged(prop_Contrast);
		}
	}

	void ColorCorrectionEffect::setSaturation(float value)
	{
		if (saturation != value) {
			saturation = value;
			raisePropertyChanged(prop_Saturation);
		}
	}

	void ColorCorrectionEffect::setTintColor(Color3 value)
	{
		if (tintColor != value) {
			tintColor = value;
			raisePropertyChanged(prop_TintColor);
		}
	}

	void ColorCorrectionEffect::setTintColor_deprecated(Color3 value)
	{
		if (tintColor_deprecated != value) {
			tintColor_deprecated = value;
			raisePropertyChanged(prop_TintColor_deprecated);
		}
	}
}
