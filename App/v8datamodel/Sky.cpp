#include "stdafx.h"

#include "./v8datamodel/Sky.h"

#include "g3d/gimage.h"
#include "util/standardout.h"
#include "v8datamodel/contentprovider.h"

const char* const RBX::sSky = "Sky";

using namespace RBX;

Reflection::PropDescriptor<Sky, TextureId> Sky::prop_Up("Up", category_Appearance, &Sky::getSkyboxUp, &Sky::setSkyboxUp);
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_Left("Left", category_Appearance, &Sky::getSkyboxLf, &Sky::setSkyboxLf);
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_Right("Right", category_Appearance, &Sky::getSkyboxRt, &Sky::setSkyboxRt);
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_Back("Back", category_Appearance, &Sky::getSkyboxBk, &Sky::setSkyboxBk);
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_Front("Front", category_Appearance, &Sky::getSkyboxFt, &Sky::setSkyboxFt);
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_Down("Down", category_Appearance, &Sky::getSkyboxDn, &Sky::setSkyboxDn);

Reflection::PropDescriptor<Sky, int> Sky::prop_StarCount("StarCount", category_Appearance, &Sky::getNumStars, &Sky::setNumStars);
Reflection::BoundProp<bool> Sky::prop_CelestialBodiesShown("CelestialBodiesShown", category_Appearance, &Sky::drawCelestialBodies);

Reflection::PropDescriptor<Sky, TextureId> Sky::prop_SkyUp("SkyboxUp", category_Appearance, &Sky::getSkyboxUp, &Sky::setSkyboxUp, Reflection::PropertyDescriptor::Attributes::deprecated(prop_Up, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING));
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_SkyLf("SkyboxLf", category_Appearance, &Sky::getSkyboxLf, &Sky::setSkyboxLf, Reflection::PropertyDescriptor::Attributes::deprecated(prop_Left, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING));
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_SkyRt("SkyboxRt", category_Appearance, &Sky::getSkyboxRt, &Sky::setSkyboxRt, Reflection::PropertyDescriptor::Attributes::deprecated(prop_Right, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING));
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_SkyBk("SkyboxBk", category_Appearance, &Sky::getSkyboxBk, &Sky::setSkyboxBk, Reflection::PropertyDescriptor::Attributes::deprecated(prop_Back, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING));
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_SkyFt("SkyboxFt", category_Appearance, &Sky::getSkyboxFt, &Sky::setSkyboxFt, Reflection::PropertyDescriptor::Attributes::deprecated(prop_Front, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING));
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_SkyDn("SkyboxDn", category_Appearance, &Sky::getSkyboxDn, &Sky::setSkyboxDn, Reflection::PropertyDescriptor::Attributes::deprecated(prop_Down, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING));

Sky::Sky()
:drawCelestialBodies(true)
,numStars(3000)
{
	setName("Sky");

    skyUp = ContentId::fromAssets("textures/sky/sky1024_up.dds");	
    skyLf = ContentId::fromAssets("textures/sky/sky1024_lf.dds");	
    skyRt = ContentId::fromAssets("textures/sky/sky1024_rt.dds");	
    skyBk = ContentId::fromAssets("textures/sky/sky1024_bk.dds");	
    skyFt = ContentId::fromAssets("textures/sky/sky1024_ft.dds");	
    skyDn = ContentId::fromAssets("textures/sky/sky1024_dn.dds");
}

void Sky::setNumStars(int value)
{
	value = std::min(value, 5000);
	value = std::max(value, 0);
	if (value != numStars)
	{
		numStars = value;
		raisePropertyChanged(prop_StarCount);
	}
}

void Sky::setSkyboxBk(const TextureId& texId)
{
    if (texId != skyBk)
    {
        skyBk = texId;
        raisePropertyChanged(prop_SkyBk);
    }
}

void Sky::setSkyboxDn(const TextureId& texId)
{
    if (texId != skyDn)
    {
        skyDn = texId;
        raisePropertyChanged(prop_SkyDn);
    }
}

void Sky::setSkyboxLf(const TextureId& texId)
{
    if (texId != skyLf)
    {
        skyLf = texId;
        raisePropertyChanged(prop_SkyLf);
    }
}

void Sky::setSkyboxRt(const TextureId& texId)
{
    if (texId != skyRt)
    {
        skyRt = texId;
        raisePropertyChanged(prop_SkyRt);
    }
}
void Sky::setSkyboxUp(const TextureId& texId)
{
    if (texId != skyUp)
    {
        skyUp = texId;
        raisePropertyChanged(prop_SkyUp);
    }
}

void Sky::setSkyboxFt(const TextureId&  texId)
{
    if (texId != skyFt)
    {
        skyFt = texId;    
        raisePropertyChanged(prop_SkyFt);
    }
}
