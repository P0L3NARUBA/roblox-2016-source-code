#pragma once

#include "V8Tree/Instance.h"
#include "PostEffect.h"

namespace RBX {
	extern const char* const sBlurEffect;

	class BlurEffect : public DescribedCreatable<BlurEffect, PostEffect, sBlurEffect>

	{
	public:
		BlurEffect();
		virtual ~BlurEffect();

		int getSize() const { return size; }
		void setSize(int value);

		static Reflection::PropDescriptor<BlurEffect, int> prop_Size;

	private:
		int size;
	};
}
