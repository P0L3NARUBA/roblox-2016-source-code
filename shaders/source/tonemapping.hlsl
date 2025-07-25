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

	return pow(saturate((x * (a * x + b)) / (x * (c * x + d) + e)), Exposure_Gamma_InverseGamma_Unused.z);
}*/

/* ACES Accurate */
float3 ACESFilm(float3 color){	
	float3x3 m1 = float3x3(
        0.59719, 0.07600, 0.02840,
        0.35458, 0.90834, 0.13383,
        0.04823, 0.01566, 0.83777
	);
	float3x3 m2 = float3x3(
        1.60475, -0.10208, -0.00327,
        -0.53108,  1.10813, -0.07276,
        -0.07367, -0.00605,  1.07602
	);

	float3 v = mul(color, m1);    
	float3 a = v * (v + 0.0245786) - 0.000090537;
	float3 b = v * (0.983729 * v + 0.4329510) + 0.238081;

	return pow(saturate(mul(a / b, m2)), Exposure_Gamma_InverseGamma_Unused.z);	
}

/* Khronos PBR Neutral */
// I seem to have messed up this implementation, as colors look immensely wrong
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
  return pow(lerp(color, newPeak * float3(1.0, 1.0, 1.0), g), Exposure_Gamma_InverseGamma_Unused.z);
}

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
	float3x3 agx_mat = float3x3(
		0.842479062253094, 0.0423282422610123, 0.0423756549057051,
		0.0784335999999992,  0.878468636469772,  0.0784336,
		0.0792237451477643, 0.0791661274605434, 0.879142973793104);

	float min_ev = -12.47393f;
	float max_ev = 4.026069f;

	val = mul(val, agx_mat);

	val = clamp(log2(val), min_ev, max_ev);
	val = (val - min_ev) / (max_ev - min_ev);

	val = agxDefaultContrastApprox(val);

	return val;
}

float3 agxEotf(float3 val) {
	float3x3 agx_mat_inv = float3x3(
		1.19687900512017, -0.0528968517574562, -0.0529716355144438,
		-0.0980208811401368, 1.15190312990417, -0.0980434501171241,
		-0.0990297440797205, -0.0989611768448433, 1.15107367264116);

	val = mul(val, agx_mat_inv);

	return val;
}

float3 agxLook(float3 val, float3x3 values, float sat) {
	float3 lw = float3(0.2126, 0.7152, 0.0722);
	float luma = dot(val, lw);

	float3 offset = values[0];
	float3 slope = values[1];
	float3 power = values[2];

	val = pow(val * slope + offset, power);

	return luma + sat * (val - luma);
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
	color = agxLook(color, float3x3(params1.xxx, params2.xyz, params1.yyy), params1.z);
	color = agxEotf(color);

	#else
	color = pow(color, Exposure_Gamma_InverseGamma_Unused.z);
	#endif
}
 
void ColorCorrection(inout float3 color) {
	float3 params1 = Parameters1.xyz;
	float3 params2 = Parameters2.xyz;

	color = params1.y * (color - 0.5) + 0.5 + params1.x;

	float grayscale = dot(color, float3(0.2126, 0.7152, 0.0722));

	color = lerp(grayscale.xxx, color.rgb, params1.z - 1.0) * params2;
}

float4 TonemappingPS( BasicVertexOutput IN ) : SV_TARGET {
	#ifdef BLOOM
	float3 color = lerp(MainTexture.Sample(MainSampler, IN.UV).rgb, BloomTexture.Sample(BloomSampler, IN.UV).rgb, Parameters1.w);
	#else
	float3 color = MainTexture.Sample(MainSampler, IN.UV).rgb;
	#endif

	Tonemapping(color);

	#if defined(COLOR_CORRECTION) && !defined(AGX)
	ColorCorrection(color);
	#endif

	return float4(color, 1.0);
}