#define GLOBALS

#include "buffers.hlsli"
#include "common.hlsli"

struct SkyAppData {
    float3 Position  : POSITION;
    float3 UVW       : TEXCOORD;
};

struct SkyVertexOutput {
    float4 Position : SV_POSITION;
    float3 UVW      : TEXCOORD;
};

#ifdef SINGLE_FACE
TEX_DECLARE2D(float4, SkyboxFace, 0);

float4 SkyFacePS(BasicVertexOutput IN) : SV_TARGET {
    float3 Skybox = SkyboxFaceTexture.Sample(SkyboxFaceSampler, IN.UV).rgb * IN.Color;
    float3 Brightness = (Skybox / (1.0 - Skybox * 0.9875)) / 8.0;

    return float4(Skybox + Brightness, 1.0);
}
#else
TEX_DECLARECUBE(float3, Skybox, 0);

SkyVertexOutput SkyVS(SkyAppData IN) {
    SkyVertexOutput OUT;

    OUT.UVW = IN.UVW;

    OUT.Position = mul(ViewProjection[0], float4(IN.Position.xyz, 1.0));
    OUT.Position.z = 0.0;

    return OUT;
}

float4 SkyPS(SkyVertexOutput IN) : SV_TARGET {
    return float4(SkyboxTexture.Sample(SkyboxSampler, IN.UVW), 1.0);
}
#endif