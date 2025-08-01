#include "stdafx.h"
#include "V8DataModel/PostEffect.h"

namespace RBX {

	extern const char* const sPostEffect = "PostEffect";

	Reflection::PropDescriptor<PostEffect, bool> PostEffect::prop_Enabled("Enabled", category_Appearance, &PostEffect::getEnabled, &PostEffect::setEnabled);

	PostEffect::PostEffect(const char* name)
		: DescribedNonCreatable<PostEffect, Instance, sPostEffect>(name)
		, enabled(false)
	{
	}

	PostEffect::~PostEffect()
	{
	}

	void PostEffect::setEnabled(bool value) {
		if (enabled != value) {
			enabled = value;
			raisePropertyChanged(prop_Enabled);
		}
	}
}
