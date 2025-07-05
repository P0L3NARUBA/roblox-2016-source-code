#pragma once

#include "V8Tree/Instance.h"
#include "PostEffect.h"

namespace RBX {
	extern const char* const sBloomEffect;

	class BloomEffect : public DescribedCreatable<BloomEffect, PostEffect, sBloomEffect>

	{
	public:
		BloomEffect();
		virtual ~BloomEffect();

		float getIntensity() const { return intensity; }
		void setIntensity(float value);

		int getSize() const { return size; }
		void setSize(int value);

		static Reflection::PropDescriptor<BloomEffect, float> prop_Intensity;
		static Reflection::PropDescriptor<BloomEffect, int> prop_Size;

	private:
		float intensity;
		int size;
	};
}
