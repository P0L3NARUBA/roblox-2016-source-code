#if defined(GLOBALS) && !defined(GLOBALS_GUARD)
#define GLOBALS_GUARD

cbuffer Globals : register(b0) {
    float4x4 ViewProjection[2];
    float4 ViewRight;
    float4 ViewUp;
    float4 ViewDirection;
    float4 CameraPosition;

    float4 AmbientColor_EnvDiffuse;
    float4 OutdoorAmbientColor_EnvSpecular;

    float4 KeyColor_KeyShadowDistance;
    float4 KeyDirection_unused;
    float4 KeyCascade[4];

    float4 FogColor_Density;
    float4 FogParams_unused;

    float4 ViewportSize_ViewportScale;
};

#endif

#if defined(PROCESSING) && !defined(PROCESSING_GUARD)
#define PROCESSING_GUARD

cbuffer Processing : register(b0) {
    float4 TextureSize_ViewportScale;
    float4 Exposure_Gamma_InverseGamma_Unused;
	float4 Parameters1;
	float4 Parameters2;
};

#endif

#if defined(MATERIAL) && !defined(MATERIAL_GUARD)
#define MATERIAL_GUARD

// TODO: Perform bitpacking on all variables for enabling/disabling stuff to condense them into a single integer.
// int4 ClearcoatEnabled_AlbedoMode_NormalMapEnabled_EmissiveMode --> int EnabledStates
// float4 CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction --> float3 CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction
struct MaterialData {
    float4 RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused;
	float4 ClearcoatEnabled_AlbedoMode_NormalMapEnabled_EmissiveMode;
    float4 IndexOfRefraction_EmissiveFactor_ParallaxFactor_ParallaxOffset;
    
	float4 CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction;
};

cbuffer MaterialData : register(b1) {
    MaterialData MaterialsData[1024];
};

#endif

#if defined(CUBEMAP_PARALLAX) && !defined(CUBEMAP_PARALLAX_GUARD)
#define CUBEMAP_PARALLAX_GUARD

struct CubemapInfo {
	float4x4 OrientedBoundingBox;
	float4 Position_Size;
};

StructuredBuffer<CubemapInfo> CubemapsInfo : register(t29);

#endif

#if defined(INSTANCED) && !defined(INSTANCED_GUARD)
#define INSTANCED_GUARD

struct ModelMatrix {
    float4x4 Model;
};

StructuredBuffer<ModelMatrix> ModelMatrixes : register(t30);

#endif

#if defined(LIGHT_LIST) && !defined(LIGHT_LIST_GUARD)
#define LIGHT_LIST_GUARD

struct GPULight {
    float4 Position_RangeSquared;
    float4 Color_Attenuation;
    float4 Direction_SubSurfaceFac;
    float2 InnerOuterAngle;
    float2 DiffSpecFac;

    float4 AxisU;
    float4 AxisV;

    float2 Type_unused;
    float2 ShadowsColored;
};

StructuredBuffer<GPULight> LightList : register(t31);

#endif