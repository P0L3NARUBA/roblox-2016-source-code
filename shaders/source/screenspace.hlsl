#include "common.h"

TEX_DECLARE2D(Texture, 0);
TEX_DECLARE2D(Mask, 1);

// .xy = gbuffer width/height, .zw = inverse gbuffer width/height
uniform float4 TextureSize;
uniform float4 Params1;
uniform float4 Params2;

#if defined(GLSL) || defined(DX11)
float4 convertPosition(float4 p, float scale)
{
	return p;
}    
#else
float4 convertPosition(float4 p, float scale)
{
	// half-pixel offset
	return p + float4(-TextureSize.z, TextureSize.w, 0, 0) * scale;
}
#endif

#ifndef GLSL
float2 convertUv(float4 p)
{
	return p.xy * float2(0.5, -0.5) + 0.5;
}
#else
float2 convertUv(float4 p)
{
	return p.xy * 0.5 + 0.5;
}
#endif


// simple pass through structure
struct VertexOutput
{
    float4 p    : POSITION;
    float2 uv   : TEXCOORD0;
};

// position and tex coord + 4 additional tex coords
struct VertexOutput_4uv
{
    float4 p    : POSITION;
    float2 uv   : TEXCOORD0;
	float4 uv12 : TEXCOORD1;
	float4 uv34 : TEXCOORD2;
};

// position and tex coord + 8 additional tex coords
struct VertexOutput_8uv
{
    float4 p    : POSITION;
    float2 uv   : TEXCOORD0;
	float4 uv12 : TEXCOORD1;
	float4 uv34 : TEXCOORD2;
	float4 uv56 : TEXCOORD3;
	float4 uv78 : TEXCOORD4;
};

VertexOutput passThrough_vs(float4 p: POSITION)
{
    VertexOutput OUT;
    OUT.p = convertPosition(p, 1);
    OUT.uv = convertUv(p);

    return OUT;
}

float4 passThrough_ps( VertexOutput IN ) : COLOR0
{
	return tex2D(Texture, IN.uv);
}

VertexOutput_4uv downsample4x4_vs(float4 p: POSITION)
{
    float2 uv = convertUv(p);

    VertexOutput_4uv OUT;
    OUT.p = convertPosition(p, 1);
    OUT.uv = uv;

	float2 uvOffset = TextureSize.zw * 0.25f;

	OUT.uv12.xy = uv + uvOffset * float2(-1, -1);
	OUT.uv12.zw = uv + uvOffset * float2(+1, -1);
	OUT.uv34.xy = uv + uvOffset * float2(-1, +1);
	OUT.uv34.zw = uv + uvOffset * float2(+1, +1);

    return OUT;
}

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

float4 imageProcess_ps( VertexOutput IN ) : COLOR0
{
	float3 color = ACESFilm(tex2D(Texture, IN.uv).rgb);

	float3 tintColor = Params2.xyz;
	float contrast = Params1.y;
	float brightness = Params1.x;
	float grayscaleLvl = -Params1.z;

	color = contrast * (color - 0.5) + 0.5 + brightness;
	float grayscale = dot(color, float3(0.2126, 0.7152, 0.0722));
	float3 separate = lerp(color.rgb, grayscale.xxx, grayscaleLvl) * tintColor;

	return float4(separate, 1.0);
}

float4 gauss(float samples, float2 uv)
{
	float2 step = Params1.xy;
	float sigma = Params1.z;

	float sigmaN1 = 1 / sqrt(2 * 3.1415926 * sigma * sigma);
	float sigmaN2 = 1 / (2 * sigma * sigma);

	// First sample is in the center and accounts for our pixel
	float4 result = tex2D(Texture, uv) * sigmaN1;
	float weight = sigmaN1;

	// Every loop iteration computes impact of 4 pixels
	// Each sample computes impact of 2 neighbor pixels, starting with the next one to the right
	// Note that we sample exactly in between pixels to leverage bilinear filtering
	for (int i = 0; i < samples; ++i)
	{
		float ix = 2 * i + 1.5;
		float iw = 2 * exp(-ix * ix * sigmaN2) * sigmaN1;

		result += (tex2D(Texture, uv + step * ix) + tex2D(Texture, uv - step * ix)) * iw;
		weight += 2 * iw;
	}

	// Since the above is an approximation of the integral with step functions, normalization compensates for the error
	return max(result / weight, 0.0) / 4.0;
}

float4 box(float samples, float2 uv)
{
	int steps = int((samples - 1.0) * 0.5);
	float2 step = Params1.xy;
	float4 result = float4(0.0, 0.0, 0.0, 0.0);

	for (int i = -steps; i <= steps; ++i)
	{
		result += tex2D(Texture, uv + step * i);
	}

	return max(result / samples, 0.0);
}

float4 blur3_ps(VertexOutput IN): COLOR0
{
	return box(3, IN.uv);
}

float4 blur5_ps(VertexOutput IN): COLOR0
{
	return box(5, IN.uv);
}

float4 blur7_ps(VertexOutput IN): COLOR0
{
	return box(7, IN.uv);
}

float4 glowApply_ps( VertexOutput IN ) : COLOR0
{
	float4 color = tex2D(Texture, IN.uv);
	return float4(color.rgb * Params1.x, color.a);
}

// this is specific glow downsample
float4 downSample4x4Glow_ps( VertexOutput_4uv IN ) : COLOR0
{
	float4 avgColor = tex2D( Texture, IN.uv12.xy );
	avgColor += tex2D( Texture, IN.uv12.zw );
	avgColor += tex2D( Texture, IN.uv34.xy );
	avgColor += tex2D( Texture, IN.uv34.zw );

	avgColor *= 0.25;
	return float4(avgColor.rgb, 1) * (1-avgColor.a);
}

float4 ShadowBlurPS(VertexOutput IN): COLOR0
{
#ifdef GLSLES
    int N = 1;
	float sigma = 0.5;
#else
    int N = 3;
	float sigma = 1.5;
#endif

	float2 step = Params1.xy;

	float sigmaN1 = 1 / sqrt(2 * 3.1415926 * sigma * sigma);
	float sigmaN2 = 1 / (2 * sigma * sigma);

    float depth = 1;
    float color = 0;
    float weight = 0;

	for (int i = -N; i <= N; ++i)
	{
        float ix = i;
		float iw = exp(-ix * ix * sigmaN2) * sigmaN1;

        float4 data = tex2D(Texture, IN.uv + step * ix);

        depth = min(depth, data.x);
        color += data.y * iw;
		weight += iw;
	}

    float mask = tex2D(Mask, IN.uv).r;

	// Since the above is an approximation of the integral with step functions, normalization compensates for the error
	return float4(depth, color * mask * (1 / weight), 0, 0);
}

