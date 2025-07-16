#define GLOBALS

#include "buffers.hlsli"
#include "common.hlsli"

#ifdef SINGLE_FACE
TEX_DECLARE2D(SkyboxFace, 0);
#else
TEX_DECLARECUBE(Skybox, 0);
#endif

struct SkyAppData {
    float3 Position  : POSITION;
    float3 UVW       : TEXCOORD;
};

struct SkyVertexOutput {
    float4 Position : SV_POSITION;
    float3 UVW      : TEXCOORD;
};

SkyVertexOutput SkyVS(SkyAppData IN) {
    SkyVertexOutput OUT;

    OUT.UVW = IN.UVW;

    OUT.Position = mul(float4(IN.Position.xyz, 1.0), ViewProjection);
    OUT.Position.z = 0.0;

    return OUT;
}

float4 SkyPS(SkyVertexOutput IN) : SV_TARGET {
    return float4(SkyboxTexture.Sample(SkyboxSampler, IN.UVW).rgb, 1.0);
}

float4 SkyFacePS(BasicVertexOutput IN) : SV_TARGET {
    float3 Skybox = SkyboxFaceTexture.Sample(SkyboxFaceSampler, IN.UV).rgb * IN.Color;
    float3 Brightness = (Skybox / (1.0 - Skybox * 0.9875)) / 8.0;

    return float4(Skybox + Brightness, 1.0);
}