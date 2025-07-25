#include "stdafx.h"
#include "V8DataModel/BloomEffect.h"

namespace RBX {

	const char* const sBloomEffect = "BloomEffect";

	Reflection::PropDescriptor<BloomEffect, float> BloomEffect::prop_Intensity("Intensity", category_Appearance, &BloomEffect::getIntensity, &BloomEffect::setIntensity);
	Reflection::PropDescriptor<BloomEffect, int>   BloomEffect::prop_Size("Size", category_Appearance, &BloomEffect::getSize, &BloomEffect::setSize);

	Reflection::PropDescriptor<BloomEffect, TextureId> BloomEffect::prop_DirtMask("Texture", "Dirt Mask", &BloomEffect::getDirtMask, &BloomEffect::setDirtMask);
	Reflection::PropDescriptor<BloomEffect, bool>      BloomEffect::prop_UseDirtMask("Use", "Dirt Mask", &BloomEffect::getUseDirtMask, &BloomEffect::setUseDirtMask);

	BloomEffect::BloomEffect()
		:DescribedCreatable<BloomEffect, PostEffect, sBloomEffect>("BloomEffect")
		, intensity(1.0f)
		, size(5)
		, useDirtMask(false)
	{
	}

	BloomEffect::~BloomEffect()
	{
	}

	void BloomEffect::setIntensity(float value) {
		value = std::min(std::max(value, 0.0f), 10.0f);

		if (intensity != value) {
			intensity = value;
			raisePropertyChanged(prop_Intensity);
		}
	}

	void BloomEffect::setSize(int value) {
		value = std::min(std::max(value, 0), 6);

		if (size != value) {
			size = value;
			raisePropertyChanged(prop_Size);
		}
	}

	void BloomEffect::setDirtMask(const TextureId& texId) {
		if (dirtMask != texId) {
			dirtMask = texId;
			raisePropertyChanged(prop_DirtMask);
		}
	}

	void BloomEffect::setUseDirtMask(bool value) {
		if (useDirtMask != value) {
			useDirtMask = value;
			raisePropertyChanged(prop_UseDirtMask);
		}
	}
}
