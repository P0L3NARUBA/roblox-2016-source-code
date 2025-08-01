#pragma once

#include "V8Tree/Instance.h"
#include "PostEffect.h"
#include "util/PostProcessing.h"

namespace RBX {
	extern const char* const sBlurEffect;

	class BlurEffect : public DescribedCreatable<BlurEffect, PostEffect, sBlurEffect> {
	public:
		BlurEffect();
		virtual ~BlurEffect();

		int32_t getSize() const { return size; }
		void setSize(int32_t value);

		FilterMode getFilter() const { return filter; }
		void setFilter(FilterMode value);

		static Reflection::PropDescriptor<BlurEffect, int32_t> prop_Size;
		static Reflection::EnumPropDescriptor<BlurEffect, FilterMode> prop_Filter;

	private:
		int32_t size;
		FilterMode filter;
	};
}
