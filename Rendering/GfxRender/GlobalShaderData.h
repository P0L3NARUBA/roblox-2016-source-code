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
			Matrix4 ViewProjection[2];	/* Index 0 = View Projection || Index 1 = Inverse View Projection */
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
			Vector4 FogColor_Density;	/* x = Color.r,       y = Color.g,     z = Color.b,        w = Density */
			Vector4 FogParams;			/* x = Sun Influence, y = Uses Skybox, z = Affects Skybox, w = 0.0     */

			/* Misc */
			Vector4 ViewportSize_ViewportScale;

			static void define(Device* device);

			void setCamera(const RenderCamera& camera);
		};

		struct GlobalProcessingData {
			Vector4 TextureSize_ViewportScale; /* xy = Viewport size || zw = 1.0f / Viewport size, or filter size */
			Vector4 Exposure_Gamma_InverseGamma_Unused;
			Vector4 Parameters1;
			Vector4 Parameters2;

			static void define(Device* device);
		};

		struct MaterialData {
			Vector4 RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused; // -1.0 means use textures/disabled, anything above 0.0 is an override
			Vector4 AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode;
			Vector4 IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset; // Index of Refraction does not apply to metallic materials

			Vector4 CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused;
		};

		/* Albedo modes:
		0 - No albedo map
		1 - Albedo alpha defines transparency
		2 - Albedo alpha defines overlay
		3 - Albedo alpha defines tinting */

		/* Emission modes:
		0 - No emission
		1 - Inherit color from base color
		2 - Inherit color from base color and albedo
		3 - Inherit color from dedicated map */

		struct GlobalMaterialData {
			MaterialData Materials[256];

			static void define(Device* device);
		};

		struct CubemapInfo {
			Matrix4 OrientedBoundingBox;
			Vector4 Position_Size; // Size of -1.0 means box mode, while anything above 0.0 means sphere mode
		};

		struct CubemapsInfo {
			std::vector<CubemapInfo> Cubemap;

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
			Vector2 InnerOuterAngle;
			Vector2 DiffSpecFac;

			Vector4 AxisU;
			Vector4 AxisV;

			Vector2 Type_unused;
			Vector2 ShadowsColored;
		};

		struct GlobalLightList {
			std::vector<GPULight> LightList;

			static void define(Device* device);

			void populateList(const std::vector<LightObject*> lights);
		};
	}
}
