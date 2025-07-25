#pragma once

#include "V8Tree/Instance.h"
#include "PostEffect.h"
#include "Util/TextureId.h"

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

		const TextureId& getDirtMask() const { return dirtMask; }
		void setDirtMask(const TextureId& texture);

		bool getUseDirtMask() const { return useDirtMask; }
		void setUseDirtMask(bool value);

		static Reflection::PropDescriptor<BloomEffect, float> prop_Intensity;
		static Reflection::PropDescriptor<BloomEffect, int>   prop_Size;

		static Reflection::PropDescriptor<BloomEffect, TextureId> prop_DirtMask;
		static Reflection::PropDescriptor<BloomEffect, bool>      prop_UseDirtMask;

	private:
		float intensity;
		int size;

		TextureId dirtMask;
		bool useDirtMask;
	};
}
