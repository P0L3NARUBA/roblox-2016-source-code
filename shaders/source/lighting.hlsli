#include "common.hlsli"

#define FUJII_CONST ( PI / 2.0 - 2.0 / 3.0 )

#define LUT_SIZE  ( 256.0 ) // LTC texture size 
#define LUT_SCALE ( (LUT_SIZE - 1.0) / LUT_SIZE )
#define LUT_BIAS  ( 0.5 / LUT_SIZE )

float3 F0ToIOR(float3 F0) {
    float3 sqrtF0 = sqrt(clamp(F0, EPSILON, INV_EPSILON));

    return (float3(1.0, 1.0, 1.0) + sqrtF0) / (float3(1.0, 1.0, 1.0) - sqrtF0);
}

/* Cheap, but inaccurate fresnel equation for both non-metals and metals (especially inaccurate for metals). Uses an F0 surface incidence value. */
float3 FresnelSchlick(float cosTheta, float F0, float Roughness) {
    float fresnel = F0 + (max(1.0 - Roughness, F0) - F0) * pow(saturate(1.0 - cosTheta), 5.0);

    return float3(fresnel, fresnel, fresnel);
}

/* More expensive, but more accurate fresnel equation for non-metals. Uses a single physically-based Index of Refraction value. */
/* Sourced from OpenPBR */
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
    float f = 0.5 * (gc * gc) * (gcp * gcp + 1.0);

    return float3(f, f, f);
}

/* Expensive, but more accurate fresnel equation for metals. Uses two physically-based Index of Refraction values, n for facing and k for extinction. */
/* Currently the albedo is converted to an IOR value and plugged into the n value, though hopefully in the future we introduce developer-controlled IOR values for metals. */
/* https://refractiveindex.info/?shelf=3d&book=metals Red: 650 nm, Green: 550 nm, Blue: 450 nm */
float3 FresnelConductor(float cosTheta, float3 n, float3 k) {
    float cosTheta2 = saturate(cosTheta);
	cosTheta2 *= cosTheta2;
    float sinTheta2 = 1.0 - cosTheta2;
    float3 n2 = n * n;
    float3 k2 = k * k;

    float3 t0 = n2 - k2 - float3(sinTheta2, sinTheta2, sinTheta2);
    float3 a2plusb2 = sqrt(t0 * t0 + 4.0 * n2 * k2);
    float3 t1 = a2plusb2 + float3(cosTheta2, cosTheta2, cosTheta2);
    float3 a = sqrt(max(0.5 * (a2plusb2 + t0), 0.0));
    float3 t2 = 2.0 * a * cosTheta;
    float3 Rs = (t1 - t2) / (t1 + t2);

    float sinTheta4 = sinTheta2 * sinTheta2;

    float3 t3 = cosTheta2 * a2plusb2 + float3(sinTheta4, sinTheta4, sinTheta4);
    float3 t4 = t2 * sinTheta2;
    float3 Rp = Rs * (t3 - t4) / (t3 + t4);

    return 0.5 * (Rp + Rs);
}

float3 Fresnel(float cosTheta, float3 IORa, float3 IORb, float IOR, float Metalness) {
    return lerp(FresnelConductor(cosTheta, IORa, IORb), FresnelDielectric(cosTheta, IOR), Metalness);
}

/* Unsure where I got this from */
/*float GGXSmithG2(float NdotL, float NdotV, float alpha2) {
    float lambdaL = sqrt(alpha2 + (1.0 - alpha2) * NdotL * NdotL);
    float lambdaV = sqrt(alpha2 + (1.0 - alpha2) * NdotV * NdotV);
    return 2.0 / (lambdaL / NdotL + lambdaV / NdotV);
}*/

/* Sourced from OpenPBR */
float GGXSmithG2(float NdotL, float NdotV, float alpha2) {
    float lambdaL = sqrt(alpha2 + (1.0 - alpha2) * NdotL * NdotL);
    float lambdaV = sqrt(alpha2 + (1.0 - alpha2) * NdotV * NdotV);

    return 2.0 * NdotL * NdotV / (lambdaL * NdotV + lambdaV * NdotL);
}

float GGXDistribution(float NDH, float roughness) {
    float a2 = roughness * roughness;
    float NdotH2 = NDH * NDH;
	
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	
    return num / (denom * denom);
}

/* Sourced from OpenPBR */
/*float GGXDistribution(float3 H, float2 roughness) {
    float2 He = H.xy / roughness;
	
    float denom = dot(He, He) + H.z * H.z;
	
    return 1.0 / (PI * roughness.x * roughness.y * denom * denom);
}*/

float OrenNayarFujiiDiffuse(float NdotV, float NdotL, float LdotV, float sigma) {
    float s = LdotV - NdotL * NdotV;
    float t = (s > 0.0) ? max(NdotL, NdotV) : 1.0;

	float fujii = PI + FUJII_CONST * sigma;
    float A = 1.0 / fujii;
    float B = sigma / fujii;

    //return (A + B * (s / t)) * PI;
    return NdotL * (A + B * (s / t));
}

// Rendering equation for direct lighting, using GGX BRDF for speculars and Oren Nayar Fujii/Lambertian for diffuse
float3 Lighting(float DiffuseStrength, float SpecularStrength, float3 Normal, float3 LightDir, float3 ViewDir, float NDV, float Radiance, float3 Albedo, float Roughness, float Metalness, float IOR) {
    float3 HalfDir = normalize(LightDir + ViewDir);
    float NDH = saturate(dot(Normal, HalfDir));
    float VDH = saturate(dot(HalfDir, ViewDir));
	float NDL = saturate(dot(Normal, LightDir));
    float Diffuse = 0.0;
    float3 Specular = float3(0.0, 0.0, 0.0);

	float3 F = Fresnel(VDH, F0ToIOR(Albedo), float3(0.0, 0.0, 0.0), IOR, Metalness);
    Roughness = max(Roughness, EPSILON);
    float Roughness2 = Roughness * Roughness;

    DiffuseStrength *= Metalness;

    if (DiffuseStrength > 0.0)
    {
        #ifdef HQ_DIFFUSE
        /* Oren Nayar Fujii Diffuse */
        Diffuse = OrenNayarFujiiDiffuse(NDV, NDL, dot(LightDir, ViewDir) * 0.5 + 0.5, Roughness) * DiffuseStrength;
        #else
        /* Lambertian Diffuse */
        Diffuse = (1.0 - dot(F, float3(0.2126, 0.7152, 0.0722))) * DiffuseStrength;
        #endif
    }

    if (SpecularStrength > 0.0)
    {
        float D = GGXDistribution(NDH, Roughness2);
        //float D = GGXDistribution(float3(dot(HalfDir, ), dot(HalfDir, ), NDH), float2(Roughness, Roughness));
        float G = GGXSmithG2(NDL, NDV, Roughness2);

        Specular = ((D * F * G) / (4.0 * NDV)) * SpecularStrength;
    }

    return (Diffuse * Albedo * Radiance + Specular) * NDL;
}