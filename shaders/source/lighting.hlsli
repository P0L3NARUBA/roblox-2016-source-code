#include "common.hlsli"

#define FUJII_CONST ( PI / 2.0 - 2.0 / 3.0 )
#define ENERGY_PRESERVING_CONST_1 ( 0.5 - 2.0 / (3.0 * PI) )
#define ENERGY_PRESERVING_CONST_2 ( 2.0 / 3.0 - 28.0 / (15.0 * PI) )

#define LUT_SIZE  ( 256.0 ) // LTC texture size 
#define LUT_SCALE ( (LUT_SIZE - 1.0) / LUT_SIZE )
#define LUT_BIAS  ( 0.5 / LUT_SIZE )

float Power2(float x) {
    return x * x;
}

/* Method to convert normal-incidence reflectivity to an Index of Refraction value.*/
/* Sourced from OpenPBR */
float3 F0ToIOR(float3 F0) {
    float3 sqrtF0 = sqrt(clamp(F0, EPSILON, INV_EPSILON));

    return (float3(1.0, 1.0, 1.0) + sqrtF0) / (float3(1.0, 1.0, 1.0) - sqrtF0);
}

/* Method to convert an Index of Refraction value to normal-incidence reflectivity.*/
/* Sourced from OpenPBR */
float IORToF0(float IOR) {
    return Power2((IOR - 1.0) / (IOR + 1.0));
}

/* Cheap, but inaccurate fresnel equation for both non-metals and metals (especially inaccurate for metals). Uses an F0 surface incidence value. */
/* Sourced from OpenPBR */
float3 FresnelSchlick(float cosTheta, float F0, float Roughness) {
    float fresnel = F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);

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
/* Sourced from OpenPBR */
float3 FresnelConductor(float cosTheta, float3 n, float3 k) {
    float cosTheta2 = Power2(saturate(cosTheta));
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

float3 FresnelCombined(float cosTheta, float3 IORa, float3 IORb, float IOR, float Metalness) {
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
    float roughness2 = roughness * roughness;
    float denom = (NDH * NDH) * (roughness2 - 1.0) + 1.0;
	
    return roughness2 / (PI * denom * denom);
}

float LambertianDiffuse(float3 Fresnel) {
    return 1.0 - dot(Fresnel, float3(0.2126, 0.7152, 0.0722));
}

/* Diffuse model that more accurately models rough surfaces than lambertian. */
/* https://mimosa-pudica.net/improved-oren-nayar.html */
float OrenNayarFujiiDiffuse(float NdotV, float NdotL, float LdotV, float Roughness) {
    float s = LdotV - NdotL * NdotV;
    float t = (s > 0.0) ? max(NdotL, NdotV) : 1.0;

	float Fujii = PI + FUJII_CONST * Roughness;
    float A = 1.0 / Fujii;
    float B = Roughness / Fujii;

    return NdotL * (A + B * (s / t));
}

float EnergyPreservingApproximation(float mu, float Roughness) {
    float mucomp = 1.0 - mu;
    float mucomp2 = mucomp * mucomp;

    // this matrix may need to be transposed if the result looks wrong, since it was originally intended for GLSL which is column-major
    const float2x2 Gcoeffs = float2x2(0.0571085289, -0.332181442,
                                      0.491881867, 0.0714429953);

    float GoverPi = dot(mul(float2(mucomp, mucomp2), Gcoeffs), float2(1.0, mucomp2));

    return (1.0 + Roughness * GoverPi) / (1.0 + ENERGY_PRESERVING_CONST_1 * Roughness);
}

float3 EnergyPreservingOrenNayarDiffuse(float3 Albedo, float Roughness, float3 IncidentRay, float3 OutgoingRay) {
    float mu_i = IncidentRay.z; // input angle cos
    float mu_o = OutgoingRay.z; // output angle cos

    float s = dot(IncidentRay, OutgoingRay) - mu_i * mu_o; // QON s term
    float sovertF = s > 0.0 ? s / max(mu_i, mu_o) : s; // FON s/t

    float AF = 1.0 / (1.0 + ENERGY_PRESERVING_CONST_1 * Roughness); // FON A coeff.
    float3 f_ss = (Albedo / PI) * AF * (1.0 + Roughness * sovertF); // single-scatter

    float avgEF = AF * (1.0 + ENERGY_PRESERVING_CONST_2 * Roughness); // avg. albedo
    float3 rho_ms = (Albedo * Albedo) * avgEF / (float3(1.0, 1.0, 1.0) - Albedo * (1.0 - avgEF));

    float EFo = EnergyPreservingApproximation(mu_o, Roughness);
    float EFi = EnergyPreservingApproximation(mu_i, Roughness);
    float3 f_ms = (rho_ms/PI) * max(EPSILON, 1.0 - EFo) * max(EPSILON, 1.0 - EFi) / max(EPSILON, 1.0 - avgEF);

    return f_ss + f_ms;
}


// Rendering equation for direct lighting, using GGX BRDF for speculars and Oren Nayar Fujii/Lambertian for diffuse
float3 Lighting(float DiffuseStrength, float SpecularStrength, float3 Normal, float3 LightDir, float3 ViewDir, float NDV, float Radiance, float3 Albedo, float Roughness, float Metalness, float IOR) {
    float3 HalfDir = normalize(LightDir + ViewDir);
    float VDH = saturate(dot(HalfDir, ViewDir));
    float NDH = saturate(dot(Normal, HalfDir));
	float NDL = saturate(dot(Normal, LightDir));
    float Diffuse = 0.0;
    float3 Specular = float3(0.0, 0.0, 0.0);

	float3 Fresnel = FresnelCombined(VDH, F0ToIOR(Albedo), float3(0.0, 0.0, 0.0), IOR, Metalness);
    Roughness = max(Roughness, EPSILON);
    float Roughness2 = Roughness * Roughness;

    DiffuseStrength *= Metalness;

    if (DiffuseStrength > 0.0) {
        #ifdef HQ_DIFFUSE
        Diffuse = OrenNayarFujiiDiffuse(NDV, NDL, dot(LightDir, ViewDir) * 0.5 + 0.5, Roughness) * Albedo * DiffuseStrength;
        #else
        Diffuse = LambertianDiffuse(Fresnel) * Albedo * DiffuseStrength;
        #endif
    }

    if (SpecularStrength > 0.0)  {
        float Distribution = GGXDistribution(NDH, Roughness);
        float Geometry = GGXSmithG2(NDL, NDV, Roughness2);

        Specular = ((Distribution * Fresnel * Geometry) / (4.0 * NDV * NDL)) * SpecularStrength;
    }

    return (Diffuse * Radiance + Specular) * NDL;
}