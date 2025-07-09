#pragma once

#include "Util/G3DCore.h"
#include "LightObject.h"

namespace RBX
{
	namespace Graphics
	{
		class Device;
		class RenderCamera;

		struct GlobalShaderData {
			/* Camera */
			Matrix4 ViewProjection;
			Vector4 ViewRight;
			Vector4 ViewUp;
			Vector4 ViewDirection;
			Vector4 CameraPosition;

			/* Environment */
			Vector4 AmbientColor_EnvDiffuse;
			Vector4 OutdoorAmbientColor_EnvSpecular;

			/* Sun */
			Vector4 KeyColor_KeyShadowDistance;
			Vector4 KeyDirection_unused;
			Vector4 KeyCascade[4];

			/* Fog */
			Vector4 FogColor_unused;
			Vector4 FogParams_unused; /* x = Density, y = Sun Influence, z = Uses Skybox, w = 0.0 */

			static void define(Device* device);

			void setCamera(const RenderCamera& camera);
		};

		struct GlobalProcessingData {
			Vector4 TextureSize_ViewportScale;
			Vector4 Exposure_Gamma_InverseGamma_Unused;
			Vector4 Parameters1;
			Vector4 Parameters2;

			static void define(Device* device);
		};

		struct MaterialData {
			Vector4 AlbedoEnabled_RoughnessOverride_MetalnessOverride_EmissionOverride; // -1 means use textures, anything 0 - 1 is an override
			Vector4 ClearcoatEnabled_NormalMapEnabled_AmbientOcclusionEnabled_ParallaxEnabled;
			Vector4 CCTintOverride_CCNormalsEnabled;
			Vector2 CCFactorOverride_CCRoughnessOverride;
			Vector2 padding;
		};

		struct GlobalMaterialData {
			MaterialData Materials[1024];

			static void define(Device* device);
		};

		struct ModelMatrix {
			Matrix4 Model;
		};

		struct ModelMatrixes {
			std::vector<ModelMatrix> Model;

			static void define(Device* device);
		};

		struct GPULight {
			Vector4 Position_RangeSquared;
			Vector4 Color_Attenuation;
			Vector4 Direction_SubSurfaceFac;
			Vector4 InnerOuterAngle_DiffSpecFac;

			Vector4 AxisU;
			Vector4 AxisV;
			
			Vector4 ShadowsColored_Type_Active;
		};
		
		struct GlobalLightList {
			std::vector<GPULight> LightList;

			static void define(Device* device);

			void populateList(const std::vector<LightObject*> lights);
		};
	}
}
