#include "common.hlsli"

BasicVertexOutput PassThroughVS(BasicAppData IN) {
    BasicVertexOutput OUT;

    OUT.UV.x = (IN.Position.x < 0.0f) ? 0.0f : 2.0f;
    OUT.UV.y = (IN.Position.y > 0.0f) ? 0.0f : 2.0f;
    OUT.Position = float4(IN.Position.xy, 0.0f, 1.0f);

    return OUT;
}

TEX_DECLARE2D(float4, Main, 0);

float4 PassThroughPS(BasicVertexOutput IN) : SV_TARGET {
	return MainTexture.Sample(PointClamp, IN.UV);
}