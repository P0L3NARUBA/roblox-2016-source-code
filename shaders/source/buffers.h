#ifdef GLOBALS
cbuffer Globals : register(b0)
{
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

    float4 FogColor;
    float4 FogParams;
};
#endif

#ifdef PROCESSING
cbuffer Processing : register(b1)
{
    float4 TextureSize_ViewportScale;
    float4 Exposure_Gamma_InverseGamma_Unused;
};
#endif

#ifdef LIGHT_LIST
struct GPULight {
    float4 Position_RangeSquared;
    float4 Color_Attenuation;
    float4 Direction_SubSurfaceFac;
    float4 InnerOuterAngle_DiffSpecFac;

    float4 AxisU;
    float4 AxisV;

    float4 ShadowsColored_Type_Active;
};

StructuredBuffer<GPULight> LightList : register(t31);
#endif