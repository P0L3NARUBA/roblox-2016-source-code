#include "stdafx.h"
#include "GlobalShaderData.h"

#include "GfxCore/Device.h"

#include "RenderCamera.h"

namespace RBX
{
	namespace Graphics
	{

		void GlobalShaderData::define(Device* device) {
			device->defineGlobalConstants(sizeof(GlobalShaderData));
		}

		void GlobalShaderData::setCamera(const RenderCamera& camera) {
			Matrix3 camMat = camera.getViewMatrix().upper3x3().transpose();

			ViewProjection = camera.getViewProjectionMatrix();
			ViewRight = Vector4(camMat.column(0), 0);
			ViewUp = Vector4(camMat.column(1), 0);
			ViewDirection = Vector4(camMat.column(2), 0);
			CameraPosition = Vector4(camera.getPosition(), 1);
		}
		
		void GlobalProcessingData::define(Device* device) {
			device->defineGlobalProcessingData(sizeof(GlobalProcessingData));
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

				float range = light->getRange();
				gpuLight.Position_RangeSquared = Vector4(light->getPosition(), range * range);
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

				gpuLight.Position_RangeSquared = Vector4(0.0, 0.0, 0.0, 0.0);
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
