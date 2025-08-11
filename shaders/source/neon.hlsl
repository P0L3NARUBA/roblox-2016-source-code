#include "common.hlsli"

float4 NeonPS(BasicMaterialVertexOutput IN) : SV_TARGET {
    return float4(IN.Color.rgb, IN.Color.a);
}