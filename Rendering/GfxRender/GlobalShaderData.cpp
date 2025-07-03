#include "stdafx.h"
#include "GlobalShaderData.h"

#include "GfxCore/Device.h"

#include "RenderCamera.h"

namespace RBX
{
	namespace Graphics
	{

		void GlobalShaderData::define(Device* device) {
			bool useG = (device->getShadingLanguage() == "hlsl");

			std::vector<ShaderGlobalConstant> ctab;

#define MEMBER(name) ctab.push_back(ShaderGlobalConstant(useG ? "_G." #name : #name, offsetof(GlobalShaderData, name), sizeof(static_cast<GlobalShaderData*>(0)->name)))

			MEMBER(ViewProjection);
			MEMBER(ViewRight);
			MEMBER(ViewUp);
			MEMBER(ViewDir);
			MEMBER(CameraPosition);

			MEMBER(AmbientColor);
			MEMBER(Lamp0Color);
			MEMBER(Lamp0Dir);
			MEMBER(Lamp1Color);

			MEMBER(FogColor);
			MEMBER(FogParams);

			MEMBER(LightBorder);
			MEMBER(LightConfig0);
			MEMBER(LightConfig1);
			MEMBER(LightConfig2);
			MEMBER(LightConfig3);

			MEMBER(FadeDistance_GlowFactor);
			MEMBER(OutlineBrightness_ShadowInfo);

			MEMBER(ShadowMatrix0);
			MEMBER(ShadowMatrix1);
			MEMBER(ShadowMatrix2);

#undef MEMBER

			device->defineGlobalConstants(sizeof(GlobalShaderData), ctab);
		}

		void GlobalShaderData::setCamera(const RenderCamera& camera) {
			Matrix3 camMat = camera.getViewMatrix().upper3x3().transpose();

			ViewProjection = camera.getViewProjectionMatrix();
			ViewRight = Vector4(camMat.column(0), 0);
			ViewUp = Vector4(camMat.column(1), 0);
			ViewDir = Vector4(camMat.column(2), 0);
			CameraPosition = Vector4(camera.getPosition(), 1);
		}

		void GlobalLightList::define(Device* device) {
			device->defineGlobalLightList(sizeof(GPULight) * 1024u, sizeof(GPULight));
		}

		void GlobalLightList::populateList(const std::vector<LightObject*> lights) {
			LightList.clear();

			size_t maxSize = std::min(lights.size(), size_t(1024u));

			for (size_t i = 0; i < maxSize; ++i) {
				LightObject* light = lights[i];
				GPULight gpuLight;

				gpuLight.Position_Range = Vector4(light->getPosition(), light->getRange());
				Color3 color = light->getColor();
				float brightness = light->getBrightness();
				float attenuation = light->getAttenuation();
				gpuLight.Color_Attenuation = Vector4(color.r * brightness, color.g * brightness, color.b * brightness, attenuation * attenuation * attenuation);
				gpuLight.Direction_SubSurfaceFac = Vector4(light->getDirection(), 0.0);
				gpuLight.InnerOuterAngle_DiffSpecFac = Vector4(light->getInnerAngle(), light->getOuterAngle(), light->getDiffuseFactor(), light->getSpecularFactor());

				gpuLight.AxisU = light->getAxisU();
				gpuLight.AxisV = light->getAxisV();

				gpuLight.ShadowsColored_Type_Active = Vector4(0, 0, 0, 1);
				
				LightList.push_back(gpuLight);
			}

			for (size_t i = maxSize; i < 1024; ++i) {
				GPULight gpuLight;

				gpuLight.Position_Range = Vector4(0.0, 0.0, 0.0, 0.0);
				gpuLight.Color_Attenuation = Vector4(0.0, 0.0, 0.0, 0.0);
				gpuLight.Direction_SubSurfaceFac = Vector4(0.0, 0.0, 0.0, 0.0);
				gpuLight.InnerOuterAngle_DiffSpecFac = Vector4(0.0, 0.0, 0.0, 0.0);

				gpuLight.AxisU = Vector4(0.0, 0.0, 0.0, 0.0);;
				gpuLight.AxisV = Vector4(0.0, 0.0, 0.0, 0.0);;

				gpuLight.ShadowsColored_Type_Active = Vector4(0.0, 0.0, 0.0, 0.0);

				LightList.push_back(gpuLight);
			}
		};
	}
}
