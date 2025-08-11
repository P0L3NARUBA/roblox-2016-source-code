#define PROCESSING

#include "buffers.hlsli"
#include "common.hlsli"

TEX_DECLARE2D(float4, Main, 0);
#ifdef BLOOM
TEX_DECLARE2D(float4, Bloom, 1);
#endif

float3 ReinhardSimple(float3 x) {
	return x / (x + 1.0);
}

/* ACES Approximation */
/*float3 ACESFilm(float3 x) {
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;

	return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}*/

#define ACES_INPUT_MATRIX float3x3(0.59719, 0.07600, 0.02840, \
							 0.35458, 0.90834, 0.13383, \
							 0.04823, 0.01566, 0.83777)
#define ACES_OUTPUT_MATRIX float3x3( 1.60475, -0.10208, -0.00327, \
									 -0.53108,  1.10813, -0.07276, \
									 -0.07367, -0.00605,  1.07602)

/* ACES Accurate */
float3 ACESFilm(float3 color) {
	float3 v = mul(color, ACES_INPUT_MATRIX);    
	float3 a = v * (v + 0.0245786) - 0.000090537;
	float3 b = v * (0.983729 * v + 0.4329510) + 0.238081;

	return saturate(mul(a / b, ACES_OUTPUT_MATRIX));
}

/* Khronos PBR Neutral */
float3 PBRNeutralToneMapping(float3 color) {
  float startCompression = 0.8 - 0.04;
  float desaturation = 0.15;

  float x = min(color.r, min(color.g, color.b));
  float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
  color -= offset;

  float peak = max(color.r, max(color.g, color.b));
  if (peak < startCompression) return color;

  float d = 1.0 - startCompression;
  float newPeak = 1.0 - d * d / (peak + d - startCompression);
  color *= newPeak / peak;

  float g = 1.0 - 1.0 / (desaturation * (peak - newPeak) + 1.0);

  return lerp(color, newPeak, g);
}

#define MIN_EV -12.47393
#define MAX_EV 4.026069

#define AGX_INPUT_MATRIX float3x3(0.842479062253094,  0.0423282422610123, 0.0423756549057051, \
    							  0.0784335999999992, 0.878468636469772,  0.0784336,			\
    							  0.0792237451477643, 0.0791661274605434, 0.879142973793104)
#define AGX_OUTPUT_MATRIX float3x3( 1.19687900512017,   -0.0528968517574562, -0.0529716355144438, \
    							   -0.0980208811401368,  1.15190312990417,   -0.0980434501171241, \
    							   -0.0990297440797205, -0.0989611768448433,  1.15107367264116)

float3 agxDefaultContrastApprox(float3 x) {
	float3 x2 = x * x;
 	float3 x4 = x2 * x2;

	return + 15.5	* x4 * x2
		   - 40.14	* x4 * x
		   + 31.96	* x4
		   - 6.868	* x2 * x
		   + 0.4298	* x2
		   + 0.1191	* x
		   - 0.00232;
}

float3 agx(float3 val) {
	val = mul(val, AGX_INPUT_MATRIX);

	val = clamp(log2(val), MIN_EV, MAX_EV);
	val = (val - MIN_EV) / (MAX_EV - MIN_EV);

	val = agxDefaultContrastApprox(val);

	return val;
}

float3 agxLook(float3 val, float offset, float3 slope, float power, float sat) {
	val = pow(val * slope, power) + offset;

	float luma = dot(val, GRAYSCALE);

	return luma + sat * (val - luma);
}

float3 agxEotf(float3 val) {
	return mul(val, AGX_OUTPUT_MATRIX);
}

void Tonemapping(inout float3 color) {
	#ifdef REINHARD
	color = ReinhardSimple(color);

	#elif ACES
	color = ACESFilm(color);

	#elif PBR_NEUTRAL
	color = PBRNeutralToneMapping(color);
	
	#elif AGX
	float3 params1 = Parameters1.xyz;
	float3 params2 = Parameters2.xyz;

	color = agx(color);
	color = agxLook(color, params1.x, params2.xyz, params1.y, params1.z);
	color = agxEotf(color);

	#endif

	#ifndef AGX
	color = pow(color, Exposure_Gamma_InverseGamma_Unused.z);
	#endif
}
 
void ColorCorrection(inout float3 color) {
	float3 params1 = Parameters1.xyz;
	float3 params2 = Parameters2.xyz;

	color = params1.y * (color - 0.5) + 0.5 + params1.x;

	float grayscale = dot(color, GRAYSCALE);

	color = lerp(grayscale.xxx, color.rgb, params1.z) * params2;
}

float4 TonemappingPS( BasicVertexOutput IN ) : SV_TARGET {
	#ifdef BLOOM
	float3 color = lerp(MainTexture.Sample(MainSampler, IN.UV).rgb, BloomTexture.Sample(BloomSampler, IN.UV).rgb, Parameters1.w);
	#else
	float3 color = MainTexture.Sample(MainSampler, IN.UV).rgb;
	#endif

	color = max(color, 0.0) * Exposure_Gamma_InverseGamma_Unused.x;

	Tonemapping(color);

	#ifndef AGX
	ColorCorrection(color);
	#endif

	return float4(color, 1.0);
}