#define PROCESSING

#include "buffers.h"
#include "common.h"

TEX_DECLARE2D(Texture, 0);

float4 InitialBloomDownsamplePS( BasicVertexOutput IN ) : SV_TARGET {
	return float4(max(tex2D(Texture, IN.UV).rgb, 0.0) / Params1.z, 1.0);
}

float4 BloomDownsamplePS( BasicVertexOutput IN) : SV_TARGET {
	float2 texCoord = IN.UV;
    float2 srcTexelSize = Params1.xy;
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;

    float3 a = tex2D(Texture, float2(texCoord.x - x * 2.0, texCoord.y + y * 2.0)).rgb;
    float3 b = tex2D(Texture, float2(texCoord.x          , texCoord.y + y * 2.0)).rgb;
    float3 c = tex2D(Texture, float2(texCoord.x + x * 2.0, texCoord.y + y * 2.0)).rgb;

    float3 d = tex2D(Texture, float2(texCoord.x - x * 2.0, texCoord.y          )).rgb;
    float3 e = tex2D(Texture, float2(texCoord.x          , texCoord.y          )).rgb;
    float3 f = tex2D(Texture, float2(texCoord.x + x * 2.0, texCoord.y          )).rgb;

    float3 g = tex2D(Texture, float2(texCoord.x - x * 2.0, texCoord.y - y * 2.0)).rgb;
    float3 h = tex2D(Texture, float2(texCoord.x          , texCoord.y - y * 2.0)).rgb;
    float3 i = tex2D(Texture, float2(texCoord.x + x * 2.0, texCoord.y - y * 2.0)).rgb;

    float3 j = tex2D(Texture, float2(texCoord.x - x      , texCoord.y + y      )).rgb;
    float3 k = tex2D(Texture, float2(texCoord.x + x      , texCoord.y + y      )).rgb;
    float3 l = tex2D(Texture, float2(texCoord.x - x      , texCoord.y - y      )).rgb;
    float3 m = tex2D(Texture, float2(texCoord.x + x      , texCoord.y - y      )).rgb;

    float3 downsample = e * 0.125;
    downsample += (a + c + g + i) * 0.03125;
    downsample += (b + d + f + h) * 0.0625;
    downsample += (j + k + l + m) * 0.125;

	return float4(downsample, 1.0);
}

float4 BloomUpsamplePS( BasicVertexOutput IN ) : SV_TARGET {
	float2 texCoord = IN.UV;
    float2 filterRadius = Params1.xy;
    float x = filterRadius.x;
    float y = filterRadius.y;

    float3 a = tex2D(Texture, float2(texCoord.x - x, texCoord.y + y)).rgb;
    float3 b = tex2D(Texture, float2(texCoord.x    , texCoord.y + y)).rgb;
    float3 c = tex2D(Texture, float2(texCoord.x + x, texCoord.y + y)).rgb;

    float3 d = tex2D(Texture, float2(texCoord.x - x, texCoord.y    )).rgb;
    float3 e = tex2D(Texture, float2(texCoord.x    , texCoord.y    )).rgb;
    float3 f = tex2D(Texture, float2(texCoord.x + x, texCoord.y    )).rgb;

    float3 g = tex2D(Texture, float2(texCoord.x - x, texCoord.y - y)).rgb;
    float3 h = tex2D(Texture, float2(texCoord.x    , texCoord.y - y)).rgb;
    float3 i = tex2D(Texture, float2(texCoord.x + x, texCoord.y - y)).rgb;

    float3 upsample = e * 4.0;
    upsample += (b + d + f + h) * 2.0;
    upsample += (a + c + g + i);
    upsample *= 1.0 / 16.0;

	return float4(upsample, 1.0);
}