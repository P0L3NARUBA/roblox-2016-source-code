#include "stdafx.h"

#include "./v8datamodel/Sky.h"

#include "g3d/gimage.h"
#include "util/standardout.h"
#include "v8datamodel/contentprovider.h"

const char* const RBX::sSky = "Sky";

using namespace RBX;

Reflection::PropDescriptor<Sky, int32_t>  Sky::prop_StarCount("StarCount", category_Appearance, &Sky::getNumStars, &Sky::setNumStars);
Reflection::PropDescriptor<Sky, bool> Sky::prop_CelestialBodiesShown("CelestialBodiesShown", category_Appearance, &Sky::getDrawCelestialBodies, &Sky::setDrawCelestialBodies);

Reflection::PropDescriptor<Sky, TextureId> Sky::prop_Up   ("Up",    "Skybox", &Sky::getSkyboxUp, &Sky::setSkyboxUp);
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_Left ("Left",  "Skybox", &Sky::getSkyboxLf, &Sky::setSkyboxLf);
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_Right("Right", "Skybox", &Sky::getSkyboxRt, &Sky::setSkyboxRt);
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_Back ("Back",  "Skybox", &Sky::getSkyboxBk, &Sky::setSkyboxBk);
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_Front("Front", "Skybox", &Sky::getSkyboxFt, &Sky::setSkyboxFt);
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_Down ("Down",  "Skybox", &Sky::getSkyboxDn, &Sky::setSkyboxDn);
Reflection::PropDescriptor<Sky, bool>      Sky::prop_Highlights("Reconstruct Highlights", "Skybox", &Sky::getReconstructHighlights, &Sky::setReconstructHighlights);

Reflection::PropDescriptor<Sky, TextureId> Sky::prop_HDRI   ("Texture", "HDRI", &Sky::getSkyboxHDRI, &Sky::setSkyboxHDRI);
Reflection::PropDescriptor<Sky, bool>      Sky::prop_UseHDRI("Use", "HDRI", &Sky::getUseHDRI, &Sky::setUseHDRI);

Reflection::PropDescriptor<Sky, TextureId> Sky::prop_SkyUp("SkyboxUp", category_Appearance, &Sky::getSkyboxUp, &Sky::setSkyboxUp, Reflection::PropertyDescriptor::Attributes::deprecated(prop_Up,    Reflection::PropertyDescriptor::HIDDEN_SCRIPTING));
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_SkyLf("SkyboxLf", category_Appearance, &Sky::getSkyboxLf, &Sky::setSkyboxLf, Reflection::PropertyDescriptor::Attributes::deprecated(prop_Left,  Reflection::PropertyDescriptor::HIDDEN_SCRIPTING));
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_SkyRt("SkyboxRt", category_Appearance, &Sky::getSkyboxRt, &Sky::setSkyboxRt, Reflection::PropertyDescriptor::Attributes::deprecated(prop_Right, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING));
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_SkyBk("SkyboxBk", category_Appearance, &Sky::getSkyboxBk, &Sky::setSkyboxBk, Reflection::PropertyDescriptor::Attributes::deprecated(prop_Back,  Reflection::PropertyDescriptor::HIDDEN_SCRIPTING));
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_SkyFt("SkyboxFt", category_Appearance, &Sky::getSkyboxFt, &Sky::setSkyboxFt, Reflection::PropertyDescriptor::Attributes::deprecated(prop_Front, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING));
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_SkyDn("SkyboxDn", category_Appearance, &Sky::getSkyboxDn, &Sky::setSkyboxDn, Reflection::PropertyDescriptor::Attributes::deprecated(prop_Down,  Reflection::PropertyDescriptor::HIDDEN_SCRIPTING));

Sky::Sky()
	:drawCelestialBodies(true)
	, numStars(3000)
	, useHDRI(true)
{
	setName("Sky");

	skyUp = ContentId::fromAssets("textures/sky/sky512_up.tex");
	skyLf = ContentId::fromAssets("textures/sky/sky512_lf.tex");
	skyRt = ContentId::fromAssets("textures/sky/sky512_rt.tex");
	skyBk = ContentId::fromAssets("textures/sky/sky512_bk.tex");
	skyFt = ContentId::fromAssets("textures/sky/sky512_ft.tex");
	skyDn = ContentId::fromAssets("textures/sky/sky512_dn.tex");

	skyHDRI = ContentId::fromAssets("textures/sky/default_hdri.dds");
}

void Sky::setNumStars(int32_t value) {
	value = std::min(value, 5000);
	value = std::max(value, 0);

	if (numStars != value) {
		numStars = value;
		raisePropertyChanged(prop_StarCount);
	}
}

void Sky::setDrawCelestialBodies(bool value) {
	if (drawCelestialBodies != value) {
		drawCelestialBodies = value;
		raisePropertyChanged(prop_CelestialBodiesShown);
	}
}

void Sky::setSkyboxBk(const TextureId& texId) {
	if (skyBk != texId) {
		skyBk = texId;
		raisePropertyChanged(prop_SkyBk);
	}
}

void Sky::setSkyboxDn(const TextureId& texId) {
	if (skyDn != texId) {
		skyDn = texId;
		raisePropertyChanged(prop_SkyDn);
	}
}

void Sky::setSkyboxLf(const TextureId& texId) {
	if (skyLf != texId) {
		skyLf = texId;
		raisePropertyChanged(prop_SkyLf);
	}
}

void Sky::setSkyboxRt(const TextureId& texId) {
	if (skyRt != texId) {
		skyRt = texId;
		raisePropertyChanged(prop_SkyRt);
	}
}

void Sky::setSkyboxUp(const TextureId& texId) {
	if (skyUp != texId) {
		skyUp = texId;
		raisePropertyChanged(prop_SkyUp);
	}
}

void Sky::setSkyboxFt(const TextureId& texId) {
	if (skyFt != texId) {
		skyFt = texId;
		raisePropertyChanged(prop_SkyFt);
	}
}

void Sky::setReconstructHighlights(bool value) {
	if (reconstructHighlights != value) {
		reconstructHighlights = value;
		raisePropertyChanged(prop_Highlights);
	}
}

void Sky::setSkyboxHDRI(const TextureId& texId) {
	if (skyHDRI != texId) {
		skyHDRI = texId;
		raisePropertyChanged(prop_HDRI);
	}
}

void Sky::setUseHDRI(bool value) {
	if (useHDRI != value) {
		useHDRI = value;
		raisePropertyChanged(prop_UseHDRI);
	}
}