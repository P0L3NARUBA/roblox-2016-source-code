#define PROCESSING

#include "buffers.hlsli"
#include "common.hlsli"

TEX_DECLARE2D(float3, Main, 0);

float4 InitialBloomDownsamplePS( BasicVertexOutput IN ) : SV_TARGET {
	return float4(max(MainTexture.Sample(PointClamp, IN.UV), 0.0f) / Parameters1.x, 1.0f);
}

float4 BloomDownsamplePS( BasicVertexOutput IN ) : SV_TARGET {
	float2 texCoord = IN.UV;
    float2 srcTexelSize = TextureSize_ViewportScale.zw;
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;

    float3 a = MainTexture.SampleLevel(LinearBorder, float2(texCoord.x - x * 2.0f, texCoord.y + y * 2.0f), Parameters1.y);
    float3 b = MainTexture.SampleLevel(LinearBorder, float2(texCoord.x           , texCoord.y + y * 2.0f), Parameters1.y);
    float3 c = MainTexture.SampleLevel(LinearBorder, float2(texCoord.x + x * 2.0f, texCoord.y + y * 2.0f), Parameters1.y);

    float3 d = MainTexture.SampleLevel(LinearBorder, float2(texCoord.x - x * 2.0f, texCoord.y           ), Parameters1.y);
    float3 e = MainTexture.SampleLevel(LinearBorder, float2(texCoord.x           , texCoord.y           ), Parameters1.y);
    float3 f = MainTexture.SampleLevel(LinearBorder, float2(texCoord.x + x * 2.0f, texCoord.y           ), Parameters1.y);

    float3 g = MainTexture.SampleLevel(LinearBorder, float2(texCoord.x - x * 2.0f, texCoord.y - y * 2.0f), Parameters1.y);
    float3 h = MainTexture.SampleLevel(LinearBorder, float2(texCoord.x           , texCoord.y - y * 2.0f), Parameters1.y);
    float3 i = MainTexture.SampleLevel(LinearBorder, float2(texCoord.x + x * 2.0f, texCoord.y - y * 2.0f), Parameters1.y);

    float3 j = MainTexture.SampleLevel(LinearBorder, float2(texCoord.x - x       , texCoord.y + y       ), Parameters1.y);
    float3 k = MainTexture.SampleLevel(LinearBorder, float2(texCoord.x + x       , texCoord.y + y       ), Parameters1.y);
    float3 l = MainTexture.SampleLevel(LinearBorder, float2(texCoord.x - x       , texCoord.y - y       ), Parameters1.y);
    float3 m = MainTexture.SampleLevel(LinearBorder, float2(texCoord.x + x       , texCoord.y - y       ), Parameters1.y);

    float3 downsample = e * 0.125f;
    downsample += (a + c + g + i) * 0.03125f;
    downsample += (b + d + f + h) * 0.0625f;
    downsample += (j + k + l + m) * 0.125f;

	return float4(downsample, 1.0f);
}

float4 BloomUpsamplePS( BasicVertexOutput IN ) : SV_TARGET {
	float2 texCoord = IN.UV;
    float2 filterRadius = TextureSize_ViewportScale.zw;
    float x = filterRadius.x;
    float y = filterRadius.y;

    float3 a = MainTexture.SampleLevel(LinearClamp, float2(texCoord.x - x, texCoord.y + y), Parameters1.y);
    float3 b = MainTexture.SampleLevel(LinearClamp, float2(texCoord.x    , texCoord.y + y), Parameters1.y);
    float3 c = MainTexture.SampleLevel(LinearClamp, float2(texCoord.x + x, texCoord.y + y), Parameters1.y);

    float3 d = MainTexture.SampleLevel(LinearClamp, float2(texCoord.x - x, texCoord.y    ), Parameters1.y);
    float3 e = MainTexture.SampleLevel(LinearClamp, float2(texCoord.x    , texCoord.y    ), Parameters1.y);
    float3 f = MainTexture.SampleLevel(LinearClamp, float2(texCoord.x + x, texCoord.y    ), Parameters1.y);

    float3 g = MainTexture.SampleLevel(LinearClamp, float2(texCoord.x - x, texCoord.y - y), Parameters1.y);
    float3 h = MainTexture.SampleLevel(LinearClamp, float2(texCoord.x    , texCoord.y - y), Parameters1.y);
    float3 i = MainTexture.SampleLevel(LinearClamp, float2(texCoord.x + x, texCoord.y - y), Parameters1.y);

    float3 upsample = e * 4.0f;
    upsample += (b + d + f + h) * 2.0f;
    upsample += (a + c + g + i);
    upsample *= 1.0f / 16.0f;

	return float4(upsample, 1.0f);
}