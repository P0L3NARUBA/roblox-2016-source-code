#define GLOBALS
#define INSTANCED

#include "buffers.hlsli"
#include "common.hlsli"

BasicVertexOutput BasicMaterialVS(InstancedBasicAppData IN) {
    BasicVertexOutput OUT;

    float4x4 ModelMatrix = IN.InstanceID;

    OUT.Position = mul(float4(IN.Position.xyz, 1.0), ModelMatrix);
    OUT.Position = mul(OUT.Position, ViewProjection);

    OUT.UV = IN.UV;
    OUT.Color = IN.Color;

    return OUT;
}

float4 NeonPS(BasicVertexOutput IN) : SV_TARGET {
    return float4(IN.Color.rgb, IN.Color.a);
}