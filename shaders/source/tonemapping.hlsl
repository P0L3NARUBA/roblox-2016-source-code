#define PROCESSING

#include "buffers.hlsli"
#include "common.hlsli"

TEX_DECLARE2D(Main, 0);
#ifdef BLOOM
TEX_DECLARE2D(Bloom, 1);
#endif

float3 ReinhardSimple(float3 x) {
	return x / (x + 1.0);
}

float3 ACESFilm(float3 x) {
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;

	return saturate((x*(a*x+b))/(x*(c*x+d)+e));
}

float3 Tonemapping(inout float3 color) {
	#ifdef REINHARD
	color = ReinhardSimple(color);
	#elif ACES
	color = ACESFilm(color);
	#elif PBR_NEUTRAL

	#elif AGX

	#endif

	return color;
}

float3 ColorCorrection(inout float3 color) {
	float3 params1 = Parameters1.xyz;
	float3 params2 = Parameters2.xyz;

	color = params1.y * (color - 0.5) + 0.5 + params1.x;

	float grayscale = dot(color, float3(0.2126, 0.7152, 0.0722));

	return lerp(color.rgb, grayscale.xxx, params1.z) * params2;
}

float4 TonemappingPS( BasicVertexOutput IN ) : SV_TARGET {
	#ifdef BLOOM
	float3 color = lerp(MainTexture.Sample(MainSampler, IN.UV).rgb, BloomTexture.Sample(BloomSampler, IN.UV).rgb, Params1.w);
	#else
	float3 color = MainTexture.Sample(MainSampler, IN.UV).rgb;
	#endif

	Tonemapping(color);

	#ifdef COLOR_CORRECTION
	ColorCorrection(color);
	#endif

	return float4(color, 1.0);
}