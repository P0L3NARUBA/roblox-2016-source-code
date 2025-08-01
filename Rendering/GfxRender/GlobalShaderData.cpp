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
			ViewRight = Vector4(camMat.column(0u), 0.0f);
			ViewUp = Vector4(camMat.column(1u), 0.0f);
			ViewDirection = Vector4(camMat.column(2u), 0.0f);
			CameraPosition = Vector4(camera.getPosition(), 1.0f);
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

			size_t maxSize = std::min(lights.size(), 1024u);

			for (size_t i = 0u; i < maxSize; ++i) {
				LightObject* light = lights[i];
				GPULight gpuLight;

				Color3 color = light->getColor();
				float range = light->getRange();
				float attenuation = light->getAttenuation();
				float brightness = light->getBrightness();

				gpuLight.Position_RangeSquared = Vector4(light->getPosition(), range * range);
				gpuLight.Color_Attenuation = Vector4(color.r * brightness, color.g * brightness, color.b * brightness, attenuation * attenuation * attenuation);
				gpuLight.Direction_SubSurfaceFac = Vector4(light->getDirection(), 0.0f);
				gpuLight.InnerOuterAngle = Vector2(light->getInnerAngle(), light->getOuterAngle());
				gpuLight.DiffSpecFac = Vector2(light->getDiffuseFactor(), light->getSpecularFactor());

				gpuLight.AxisU = light->getAxisU();
				gpuLight.AxisV = light->getAxisV();

				// TODO: Add 32 bit integer vectors
				gpuLight.ShadowsColored = Vector2(0.0f, 0.0f);
				gpuLight.Type_unused = Vector2(0.0f, 0.0f);
				
				LightList.push_back(gpuLight);
			}
		};
	}
}
