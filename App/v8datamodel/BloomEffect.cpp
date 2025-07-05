#include "stdafx.h"
#include "V8DataModel/BloomEffect.h"

namespace RBX {

	const char* const sBloomEffect = "BloomEffect";

	Reflection::PropDescriptor<BloomEffect, float> BloomEffect::prop_Intensity("Intensity", category_Appearance, &BloomEffect::getIntensity, &BloomEffect::setIntensity);
	Reflection::PropDescriptor<BloomEffect, int> BloomEffect::prop_Size("Size", category_Appearance, &BloomEffect::getSize, &BloomEffect::setSize);

	BloomEffect::BloomEffect()
		:DescribedCreatable<BloomEffect, PostEffect, sBloomEffect>("BloomEffect")
		, intensity(1.0f)
		, size(5)
	{
	}

	BloomEffect::~BloomEffect()
	{
	}

	void BloomEffect::setIntensity(float value)
	{
		value = std::min(std::max(value, 0.0f), 10.0f);

		if (intensity != value) {
			intensity = value;
			raisePropertyChanged(prop_Intensity);
		}
	}

	void BloomEffect::setSize(int value)
	{
		value = std::min(std::max(value, 0), 6);

		if (size != value) {
			size = value;
			raisePropertyChanged(prop_Size);
		}
	}
}
