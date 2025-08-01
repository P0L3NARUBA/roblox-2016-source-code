#include "stdafx.h"
#include "V8DataModel/BlurEffect.h"

namespace RBX {

	const char* const sBlurEffect = "BlurEffect";

	namespace Reflection {
		template<>
		EnumDesc<FilterMode>::EnumDesc()
			:EnumDescriptor("Filter")
		{
			addPair(BOX, "Box");
			addPair(GAUSSIAN, "Gaussian");
			addPair(DIRECTIONAL, "Directional");
			addPair(RADIAL, "Radial");
		}
	}

	Reflection::PropDescriptor<BlurEffect, int32_t>		   BlurEffect::prop_Size("Size", category_Appearance, &BlurEffect::getSize, &BlurEffect::setSize);
	Reflection::EnumPropDescriptor<BlurEffect, FilterMode> BlurEffect::prop_Filter("Filter", category_Appearance, &BlurEffect::getFilter, &BlurEffect::setFilter);

	BlurEffect::BlurEffect()
		: DescribedCreatable<BlurEffect, PostEffect, sBlurEffect>("BlurEffect")
		, size(5u)
		, filter(GAUSSIAN)
	{
	}

	BlurEffect::~BlurEffect()
	{
	}

	void BlurEffect::setSize(int32_t value) {
		value = std::min(std::max(value, 0), 56);

		if (size != value) {
			size = value;
			raisePropertyChanged(prop_Size);
		}
	}

	void BlurEffect::setFilter(FilterMode value) {
		if (filter != value) {
			filter = value;
			raisePropertyChanged(prop_Filter);
		}
	}
}