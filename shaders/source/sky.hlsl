#define GLOBALS

#include "buffers.hlsli"
#include "common.hlsli"

#ifdef SINGLE_FACE
TEX_DECLARE2D(float3, SkyboxFace, 0);

float4 SkyFacePS(BasicVertexOutput IN) : SV_TARGET {
    float3 Skybox = pow(SkyboxFaceTexture.Sample(SkyboxFaceSampler, IN.UV), 2.2) * IN.Color;
    Skybox = Skybox / (1.0 - Skybox * 0.9875); // Inverse Reinhard Operator, by pure coincidence :p

    return float4(Skybox, 1.0);
}

#else
struct SkyAppData {
    float3 Position  : POSITION;
};

struct SkyVertexOutput {
    centroid float4 Position : SV_POSITION;
             float3 UVW      : TEXCOORD;
};

SkyVertexOutput SkyVS(SkyAppData IN) {
    SkyVertexOutput OUT;

    OUT.Position = mul(float4(IN.Position + CameraPosition.xyz, 1.0), ViewProjection[0]);
    OUT.Position.z = 0.0;

    OUT.UVW = IN.Position;
    OUT.UVW.x = -OUT.UVW.x;

    return OUT;
}

#ifdef EQUIRECTANGULAR
TEX_DECLARE2D(float3, SkyboxHDRI, 0);

float2 SampleSphericalMap(float3 V) {
    float2 UV = float2(atan2(V.z, V.x), asin(V.y));
    UV *= float2(0.1591, -0.3183); // Inverse arc tangent
    UV += 0.5;

    return UV;
}

float4 SkyFacePS(SkyVertexOutput IN) : SV_TARGET {
    return float4(SkyboxHDRITexture.Sample(SkyboxHDRISampler, SampleSphericalMap(normalize(IN.UVW))), 1.0);
}

#else
TEX_DECLARECUBE(float3, Skybox, 0);

float4 SkyPS(SkyVertexOutput IN) : SV_TARGET {
    return float4(SkyboxTexture.SampleLevel(SkyboxSampler, IN.UVW, 0.0), 1.0);
}

#endif
#endif