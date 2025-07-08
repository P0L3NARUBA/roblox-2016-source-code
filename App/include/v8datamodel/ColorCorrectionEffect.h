#pragma once

#include "V8Tree/Instance.h"
#include "PostEffect.h"

namespace RBX {
	extern const char* const sColorCorrectionEffect;

	class ColorCorrectionEffect : public DescribedCreatable<ColorCorrectionEffect, PostEffect, sColorCorrectionEffect>

	{
	public:
		ColorCorrectionEffect();
		virtual ~ColorCorrectionEffect();

		float getBrightness() const { return brightness; }
		void setBrightness(float value);

		float getContrast() const { return contrast; }
		void setContrast(float value);

		float getSaturation() const { return saturation; }
		void setSaturation(float value);

		Color3 getTintColor() const { return tintColor; }
		void setTintColor(Color3 value);

		Color3 getTintColor_deprecated() const { return tintColor_deprecated; }
		void setTintColor_deprecated(Color3 value);

		static Reflection::PropDescriptor<ColorCorrectionEffect, float> prop_Brightness;
		static Reflection::PropDescriptor<ColorCorrectionEffect, float> prop_Contrast;
		static Reflection::PropDescriptor<ColorCorrectionEffect, float> prop_Saturation;
		static Reflection::PropDescriptor<ColorCorrectionEffect, Color3> prop_TintColor;

		static Reflection::PropDescriptor<ColorCorrectionEffect, Color3> prop_TintColor_deprecated;

	private:
		float brightness;
		float contrast;
		float saturation;
		Color3 tintColor;

		Color3 tintColor_deprecated;
	};
}
