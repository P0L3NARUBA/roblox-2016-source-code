#define PROCESSING

#include "buffers.h"
#include "common.h"

#ifdef BLOOM
TEX_DECLARE2D(BloomTexture, 1);
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
	float3 tintColor = Params2.xyz;
	float brightness = Params1.x;
	float contrast = Params1.y;
	float grayscaleLvl = Params1.z;

	color = contrast * (color - 0.5) + 0.5 + brightness;
	float grayscale = dot(color, float3(0.2126, 0.7152, 0.0722));

	return lerp(color.rgb, grayscale.xxx, grayscaleLvl) * tintColor;
}

float4 TonemappingPS( BasicVertexOutput IN, out float4 OUT : SV_TARGET ) {
	#ifdef BLOOM
	float3 color = lerp(tex2D(Texture, IN.UV).rgb, tex2D(BloomTexture, IN.UV).rgb, Params1.w);
	#else
	float3 color = tex2D(Texture, IN.UV).rgb;
	#endif

	Tonemapping(color);

	#ifdef COLOR_CORRECTION
	ColorCorrection(color);
	#endif

	OUT = float4(color, 1.0);
}