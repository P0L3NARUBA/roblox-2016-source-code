#include "common.h"

#define FUJII_CONST = PI / 2.0 - 2.0 / 3.0;

float3 FresnelDielectric(float cosTheta, float ior) {
    float c = cosTheta;
    float g2 = ior * ior + c * c - 1.0;

    if (g2 < 0.0) {
        // Total internal reflection
        return float3(1.0, 1.0, 1.0);
    }

    float g = sqrt(g2);
	float gpc = g + c;
	float gnc = g - c;
	float gc = gnc / gpc;
	float gcp = (gpc * c - 1.0) / (gnc * c + 1.0);
    float f = 0.5 * (gc * gc) * (1.0 + (gcp * gcp));

    return float3(f, f, f);
}

float GGXSmithG2(float NdotL, float NdotV, float alpha2)
{
    float lambdaL = sqrt(alpha2 + (1.0 - alpha2) * NdotL * NdotL);
    float lambdaV = sqrt(alpha2 + (1.0 - alpha2) * NdotV * NdotV);
    return 2.0 / (lambdaL / NdotL + lambdaV / NdotV);
}

float GGXDistribution(float NDH, float roughness) {
    float a2 = roughness * roughness;
    float NdotH2 = NDH * NDH;
	
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	
    return num / (denom * denom);
}

float OrenNayarFujiiDiffuse(float NdotV, float NdotL, float LdotV, float sigma) {
    float s = LdotV - NdotL * NdotV;
    float t = (s > 0.0) ? max(NdotL, NdotV) : 1.0;

	float fujii = PI + FUJII_CONST * sigma;
    float A = 1.0 / fujii;
    float B = sigma / fujii;

    return (A + B * (s / t)) * PI;
}

// Rendering equation for direct lighting, using GGX BRDF for speculars and Oren Nayar Fujii/Lambertian for diffuse
float3 Lighting(float DiffuseStrength, float SpecularStrength, float3 Normal, float3 LightDir, float3 ViewDir, float NDV, float Radiance, float3 Albedo, float Roughness, float IOR) {
    float3 HalfDir = normalize(LightDir + ViewDir);
    float NDH = saturate(dot(Normal, HalfDir));
    float VDH = saturate(dot(HalfDir, ViewDir));
	float NDL = saturate(dot(Normal, LightDir));
    float Diffuse = 0.0;
    float Specular = 0.0;

    float3 Fresnel = FresnelDielectric(VDH, IOR);
    Roughness = max(Roughness, EPSILON);
    float Roughness2 = Roughness * Roughness;

    if (DiffuseStrength > 0.0)
    {
        #ifdef HQ_DIFFUSE
        /* Oren Nayar Fujii Diffuse */
        Diffuse = OrenNayarFujiiDiffuse(NDV, NDL, (dot(LightDir, ViewDir) + 1.0) / 2.0, Roughness) * DiffuseStrength;
        #else
        /* Lambertian Diffuse */
        Diffuse = (1.0 - Fresnel) * Albedo * DiffuseStrength;
        #endif
    }

    if (SpecularStrength > 0.0)
    {
        float D = GGXDistribution(NDH, Roughness2);
        float G = GGXSmithG2(NDL, NDV, Roughness2);

        Specular = ((D * Fresnel.x * G) / (4.0 * NDV)) * SpecularStrength;
    }

    return (Diffuse * Albedo * Radiance + Specular) * NDL;
}