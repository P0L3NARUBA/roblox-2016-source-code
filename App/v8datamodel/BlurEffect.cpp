#include "stdafx.h"
#include "V8DataModel/BlurEffect.h"

namespace RBX {

	const char* const sBlurEffect = "BlurEffect";

	Reflection::PropDescriptor<BlurEffect, int> BlurEffect::prop_Size("Size", category_Appearance, &BlurEffect::getSize, &BlurEffect::setSize);

	BlurEffect::BlurEffect()
		: DescribedCreatable<BlurEffect, PostEffect, sBlurEffect>("BlurEffect")
		, size(5)
	{
	}

	BlurEffect::~BlurEffect()
	{
	}

	void BlurEffect::setSize(int value)
	{
		value = std::min(std::max(value, 0), 56);

		if (size != value) {
			size = value;
			raisePropertyChanged(prop_Size);
		}
	}
}