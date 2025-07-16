#if defined(GLOBALS) && !defined(GLOBALS_GUARD)
#define GLOBALS_GUARD

cbuffer Globals : register(b0) {
    float4x4 ViewProjection;
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

struct MaterialData {
    float4 AlbedoEnabled_RoughnessOverride_MetalnessOverride_EmissionOverride;
	float4 ClearcoatEnabled_NormalMapEnabled_AmbientOcclusionEnabled_ParallaxEnabled;
    float2 padding;
    
	float2 CCFactorOverride_CCRoughnessOverride;
	float4 CCTintOverride_CCNormalsEnabled;
};

cbuffer MaterialData : register(b1) {
    MaterialData MaterialsData[1024];
};

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