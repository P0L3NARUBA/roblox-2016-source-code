#include "V8Tree/Instance.h"
#include "Reflection/Reflection.h"
#pragma once

namespace RBX {

	extern const char* const sPostEffect;
	class PostEffect : public DescribedNonCreatable<PostEffect, Instance, sPostEffect>
	{
	public:
		explicit PostEffect(const char* name);
		virtual ~PostEffect();

		bool getEnabled() const { return enabled; }
		void setEnabled(bool value);

		static Reflection::PropDescriptor<PostEffect, bool> prop_Enabled;
	private:
		bool enabled;
	};

}
