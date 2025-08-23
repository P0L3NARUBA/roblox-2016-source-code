#define PROCESSING

#include "buffers.hlsli"
#include "common.hlsli"

TEX_DECLARE2D(float4, Main, 0);
#ifdef BLOOM
TEX_DECLARE2D(float4, Bloom, 1);
#endif

float3 ReinhardSimple(float3 x) {
	return x / (x + 1.0f);
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

#define ACES_INPUT_MATRIX float3x3(0.59719f, 0.07600f, 0.02840f, \
								   0.35458f, 0.90834f, 0.13383f, \
								   0.04823f, 0.01566f, 0.83777f)
#define ACES_OUTPUT_MATRIX float3x3( 1.60475f, -0.10208f, -0.00327f, \
									-0.53108f,  1.10813f, -0.07276f, \
									-0.07367f, -0.00605f,  1.07602f)

/* ACES Accurate */
float3 ACESFilm(float3 color) {
	float3 v = mul(color, ACES_INPUT_MATRIX);    
	float3 a = v * (v + 0.0245786f) - 0.000090537f;
	float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;

	return saturate(mul(a / b, ACES_OUTPUT_MATRIX));
}

/* Khronos PBR Neutral */
float3 PBRNeutralToneMapping(float3 color) {
  float startCompression = 0.8f - 0.04f;
  float desaturation = 0.15f;

  float x = min(color.r, min(color.g, color.b));
  float offset = x < 0.08f ? x - 6.25f * x * x : 0.04f;
  color -= offset;

  float peak = max(color.r, max(color.g, color.b));
  if (peak < startCompression) return color;

  float d = 1.0f - startCompression;
  float newPeak = 1.0f - d * d / (peak + d - startCompression);
  color *= newPeak / peak;

  float g = 1.0f - 1.0f / (desaturation * (peak - newPeak) + 1.0f);

  return lerp(color, newPeak, g);
}

#define MIN_EV -12.47393f
#define MAX_EV 4.026069f

#define AGX_INPUT_MATRIX float3x3(0.842479062253094f,  0.0423282422610123f, 0.0423756549057051f, \
    							  0.0784335999999992f, 0.878468636469772f,  0.0784336f,			\
    							  0.0792237451477643f, 0.0791661274605434f, 0.879142973793104f)
#define AGX_OUTPUT_MATRIX float3x3( 1.19687900512017f,   -0.0528968517574562f, -0.0529716355144438f, \
    							   -0.0980208811401368f,  1.15190312990417f,   -0.0980434501171241f, \
    							   -0.0990297440797205f, -0.0989611768448433f,  1.15107367264116f)

float3 agxDefaultContrastApprox(float3 x) {
	float3 x2 = x * x;
 	float3 x4 = x2 * x2;

	return + 15.5f	 * x4 * x2
		   - 40.14f	 * x4 * x
		   + 31.96f	 * x4
		   - 6.868f	 * x2 * x
		   + 0.4298f * x2
		   + 0.1191f * x
		   - 0.00232f;
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

	color = params1.y * (color - 0.5f) + 0.5f + params1.x;

	float grayscale = dot(color, GRAYSCALE);

	color = lerp(grayscale.xxx, color.rgb, params1.z) * params2;
}

float4 TonemappingPS( BasicVertexOutput IN ) : SV_TARGET {
	#ifdef BLOOM
	float3 color = lerp(MainTexture.Sample(PointClamp, IN.UV).rgb, BloomTexture.Sample(PointClamp, IN.UV).rgb, Parameters1.w);
	#else
	float3 color = MainTexture.Sample(PointClamp, IN.UV).rgb;
	#endif

	color = max(color, 0.0) * Exposure_Gamma_InverseGamma_Unused.x;

	Tonemapping(color);

	#ifndef AGX
	ColorCorrection(color);
	#endif

	return float4(color, 1.0f);
}