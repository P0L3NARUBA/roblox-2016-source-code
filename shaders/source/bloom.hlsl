#define PROCESSING

#include "buffers.hlsli"
#include "common.hlsli"

TEX_DECLARE2D(Main, 0);

float4 InitialBloomDownsamplePS( BasicVertexOutput IN ) : SV_TARGET {
	return float4(max(MainTexture.Sample(MainSampler, IN.UV).rgb, 0.0) / Parameters1.x, 1.0);
}

float4 BloomDownsamplePS( BasicVertexOutput IN ) : SV_TARGET {
	float2 texCoord = IN.UV;
    float2 srcTexelSize = TextureSize_ViewportScale.zw;
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;

    float3 a = MainTexture.Sample(MainSampler, float2(texCoord.x - x * 2.0, texCoord.y + y * 2.0)).rgb;
    float3 b = MainTexture.Sample(MainSampler, float2(texCoord.x          , texCoord.y + y * 2.0)).rgb;
    float3 c = MainTexture.Sample(MainSampler, float2(texCoord.x + x * 2.0, texCoord.y + y * 2.0)).rgb;

    float3 d = MainTexture.Sample(MainSampler, float2(texCoord.x - x * 2.0, texCoord.y          )).rgb;
    float3 e = MainTexture.Sample(MainSampler, float2(texCoord.x          , texCoord.y          )).rgb;
    float3 f = MainTexture.Sample(MainSampler, float2(texCoord.x + x * 2.0, texCoord.y          )).rgb;

    float3 g = MainTexture.Sample(MainSampler, float2(texCoord.x - x * 2.0, texCoord.y - y * 2.0)).rgb;
    float3 h = MainTexture.Sample(MainSampler, float2(texCoord.x          , texCoord.y - y * 2.0)).rgb;
    float3 i = MainTexture.Sample(MainSampler, float2(texCoord.x + x * 2.0, texCoord.y - y * 2.0)).rgb;

    float3 j = MainTexture.Sample(MainSampler, float2(texCoord.x - x      , texCoord.y + y      )).rgb;
    float3 k = MainTexture.Sample(MainSampler, float2(texCoord.x + x      , texCoord.y + y      )).rgb;
    float3 l = MainTexture.Sample(MainSampler, float2(texCoord.x - x      , texCoord.y - y      )).rgb;
    float3 m = MainTexture.Sample(MainSampler, float2(texCoord.x + x      , texCoord.y - y      )).rgb;

    float3 downsample = e * 0.125;
    downsample += (a + c + g + i) * 0.03125;
    downsample += (b + d + f + h) * 0.0625;
    downsample += (j + k + l + m) * 0.125;

	return float4(downsample, 1.0);
}

float4 BloomUpsamplePS( BasicVertexOutput IN ) : SV_TARGET {
	float2 texCoord = IN.UV;
    float2 filterRadius = TextureSize_ViewportScale.zw;
    float x = filterRadius.x;
    float y = filterRadius.y;

    float3 a = MainTexture.Sample(MainSampler, float2(texCoord.x - x, texCoord.y + y)).rgb;
    float3 b = MainTexture.Sample(MainSampler, float2(texCoord.x    , texCoord.y + y)).rgb;
    float3 c = MainTexture.Sample(MainSampler, float2(texCoord.x + x, texCoord.y + y)).rgb;

    float3 d = MainTexture.Sample(MainSampler, float2(texCoord.x - x, texCoord.y    )).rgb;
    float3 e = MainTexture.Sample(MainSampler, float2(texCoord.x    , texCoord.y    )).rgb;
    float3 f = MainTexture.Sample(MainSampler, float2(texCoord.x + x, texCoord.y    )).rgb;

    float3 g = MainTexture.Sample(MainSampler, float2(texCoord.x - x, texCoord.y - y)).rgb;
    float3 h = MainTexture.Sample(MainSampler, float2(texCoord.x    , texCoord.y - y)).rgb;
    float3 i = MainTexture.Sample(MainSampler, float2(texCoord.x + x, texCoord.y - y)).rgb;

    float3 upsample = e * 4.0;
    upsample += (b + d + f + h) * 2.0;
    upsample += (a + c + g + i);
    upsample *= 1.0 / 16.0;

	return float4(upsample, 1.0);
}