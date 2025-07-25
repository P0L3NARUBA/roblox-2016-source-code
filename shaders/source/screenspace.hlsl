#include "common.hlsli"

BasicVertexOutput PassThroughVS( BasicAppData IN ) {
    BasicVertexOutput OUT;
	
    OUT.UV = IN.UV;
	OUT.UV.y = 1.0 - OUT.UV.y;
	OUT.Color = IN.Color;
    OUT.Position = float4(IN.Position, 1.0);

    return OUT;
}

TEX_DECLARE2D(float4, Main, 0);

float4 PassThroughPS( BasicVertexOutput IN ) : SV_TARGET {
	return MainTexture.Sample(MainSampler, IN.UV) * IN.Color;
}

/*float4 gauss(float samples, float2 uv)
{
	float2 step = Params1.xy;
	float sigma = Params1.z;

	float sigmaN1 = 1 / sqrt(2 * 3.1415927 * sigma * sigma);
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

float4 blur3_ps(BasicVertexOutput IN): COLOR0
{
	return box(3, IN.uv);
}

float4 blur5_ps(BasicVertexOutput IN): COLOR0
{
	return box(5, IN.uv);
}

float4 blur7_ps(BasicVertexOutput IN): COLOR0
{
	return box(7, IN.uv);
}*/