#include "common.h"

TEX_DECLARECUBE(Skybox, 0);

struct SkyAppData {
    float3 UVW       : TEXCOORD;
    float3 Position  : POSITION;
};

struct SkyVertexOutput {
    float3 UVW      : TEXCOORD;
    float4 Position : SV_POSITION;
};

SkyVertexOutput SkyVS(SkyAppData IN) {
    SkyVertexOutput OUT;

    OUT.UVW = IN.Position.xyz;

    OUT.Position = ViewProjection * float4(IN.Position, 1.0);
    OUT.Position.z = 0.0;

    return OUT;
}

float4 SkyPS(SkyVertexOutput IN) : SV_TARGET {
    return float4(texCUBE(Skybox, IN.UVW).rgb, 1.0);
}

float4 SkyFacePS(BasicVertexOutput IN) : SV_TARGET {
    float3 Skybox = tex2D(SkyboxFace, IN.UV).rgb * IN.Color;
    float3 Brightness = (Skybox / (1.0 - Skybox * 0.9875)) / 8.0;

    return float4(Skybox + Brightness, 1.0);
}