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

			Matrix4 viewProjMatrix = camera.getViewProjectionMatrix();

			ViewProjection[0] = viewProjMatrix;
			ViewProjection[1] = viewProjMatrix.inverse();
			ViewRight = Vector4(camMat.column(0), 0.0);
			ViewUp = Vector4(camMat.column(1), 0.0);
			ViewDirection = Vector4(camMat.column(2), 0.0);
			CameraPosition = Vector4(camera.getPosition(), 1.0);
		}
		
		void GlobalProcessingData::define(Device* device) {
			device->defineGlobalProcessingData(sizeof(GlobalProcessingData));
		}

		void GlobalMaterialData::define(Device* device) {
			device->defineGlobalMaterialData(sizeof(GlobalMaterialData));
		}

		void ModelMatrixes::define(Device* device) {
			device->defineInstancedModelMatrixes(sizeof(ModelMatrix) * 8192u, sizeof(ModelMatrix));
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
				gpuLight.InnerOuterAngle = Vector2(light->getInnerAngle(), light->getOuterAngle());
				gpuLight.DiffSpecFac = Vector2(light->getDiffuseFactor(), light->getSpecularFactor());

				gpuLight.AxisU = light->getAxisU();
				gpuLight.AxisV = light->getAxisV();

				gpuLight.ShadowsColored = Vector2(0, 0);
				gpuLight.Type_unused = Vector2(0, 0);
				
				LightList.push_back(gpuLight);
			}
		};
	}
}
