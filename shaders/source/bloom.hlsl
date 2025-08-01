#define PROCESSING

#include "buffers.hlsli"
#include "common.hlsli"

TEX_DECLARE2D(float3, Main, 0);

float4 InitialBloomDownsamplePS( BasicVertexOutput IN ) : SV_TARGET {
	return float4(max(MainTexture.Sample(MainSampler, IN.UV), 0.0) / Parameters1.x, 1.0);
}

float4 BloomDownsamplePS( BasicVertexOutput IN ) : SV_TARGET {
	float2 texCoord = IN.UV;
    float2 srcTexelSize = TextureSize_ViewportScale.zw;
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;

    float3 a = MainTexture.SampleLevel(MainSampler, float2(texCoord.x - x * 2.0, texCoord.y + y * 2.0), Parameters1.y);
    float3 b = MainTexture.SampleLevel(MainSampler, float2(texCoord.x          , texCoord.y + y * 2.0), Parameters1.y);
    float3 c = MainTexture.SampleLevel(MainSampler, float2(texCoord.x + x * 2.0, texCoord.y + y * 2.0), Parameters1.y);

    float3 d = MainTexture.SampleLevel(MainSampler, float2(texCoord.x - x * 2.0, texCoord.y          ), Parameters1.y);
    float3 e = MainTexture.SampleLevel(MainSampler, float2(texCoord.x          , texCoord.y          ), Parameters1.y);
    float3 f = MainTexture.SampleLevel(MainSampler, float2(texCoord.x + x * 2.0, texCoord.y          ), Parameters1.y);

    float3 g = MainTexture.SampleLevel(MainSampler, float2(texCoord.x - x * 2.0, texCoord.y - y * 2.0), Parameters1.y);
    float3 h = MainTexture.SampleLevel(MainSampler, float2(texCoord.x          , texCoord.y - y * 2.0), Parameters1.y);
    float3 i = MainTexture.SampleLevel(MainSampler, float2(texCoord.x + x * 2.0, texCoord.y - y * 2.0), Parameters1.y);

    float3 j = MainTexture.SampleLevel(MainSampler, float2(texCoord.x - x      , texCoord.y + y      ), Parameters1.y);
    float3 k = MainTexture.SampleLevel(MainSampler, float2(texCoord.x + x      , texCoord.y + y      ), Parameters1.y);
    float3 l = MainTexture.SampleLevel(MainSampler, float2(texCoord.x - x      , texCoord.y - y      ), Parameters1.y);
    float3 m = MainTexture.SampleLevel(MainSampler, float2(texCoord.x + x      , texCoord.y - y      ), Parameters1.y);

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

    float3 a = MainTexture.SampleLevel(MainSampler, float2(texCoord.x - x, texCoord.y + y), Parameters1.y);
    float3 b = MainTexture.SampleLevel(MainSampler, float2(texCoord.x    , texCoord.y + y), Parameters1.y);
    float3 c = MainTexture.SampleLevel(MainSampler, float2(texCoord.x + x, texCoord.y + y), Parameters1.y);

    float3 d = MainTexture.SampleLevel(MainSampler, float2(texCoord.x - x, texCoord.y    ), Parameters1.y);
    float3 e = MainTexture.SampleLevel(MainSampler, float2(texCoord.x    , texCoord.y    ), Parameters1.y);
    float3 f = MainTexture.SampleLevel(MainSampler, float2(texCoord.x + x, texCoord.y    ), Parameters1.y);

    float3 g = MainTexture.SampleLevel(MainSampler, float2(texCoord.x - x, texCoord.y - y), Parameters1.y);
    float3 h = MainTexture.SampleLevel(MainSampler, float2(texCoord.x    , texCoord.y - y), Parameters1.y);
    float3 i = MainTexture.SampleLevel(MainSampler, float2(texCoord.x + x, texCoord.y - y), Parameters1.y);

    float3 upsample = e * 4.0;
    upsample += (b + d + f + h) * 2.0;
    upsample += (a + c + g + i);
    upsample *= 1.0 / 16.0;

	return float4(upsample, 1.0);
}