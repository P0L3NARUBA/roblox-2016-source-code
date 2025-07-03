cbuffer Globals : register(b0)
{
    float4x4 ViewProjection;

    float4 ViewRight;
    float4 ViewUp;
    float4 ViewDir;
    float3 CameraPosition;

    float3 AmbientColor;
    float3 Lamp0Color;
    float3 Lamp0Dir;
    float3 Lamp1Color;

    float3 FogColor;
    float4 FogParams;

    float4 LightBorder;
    float4 LightConfig0;
    float4 LightConfig1;
    float4 LightConfig2;
    float4 LightConfig3;

    float4 FadeDistance_GlowFactor;
    float4 OutlineBrightness_ShadowInfo;

    float4 ShadowMatrix0;
    float4 ShadowMatrix1;
    float4 ShadowMatrix2;
};

struct GPULight {
    float4 Position_Range;
    float4 Color_Attenuation;
    float4 Direction_SubSurfaceFac;
    float4 InnerOuterAngle_DiffSpecFac;

    float4 AxisU;
    float4 AxisV;

    float4 ShadowsColored_Type_Active;
};

#define MAX_LIGHTS 1024

StructuredBuffer<GPULight> LightList : register(t15);