#define MATERIAL
#include "default.hlsl"
#include "lighting.hlsli"
#include "parallax.hlsli"

/* Main Textures */
TEX_DECLARE2D(float4, ShadowAtlas, 0);
TEX_DECLARE2D(float4, SunShadowArray, 1);
TEX_DECLARE2D(float2, EnvironmentBRDF, 2); // may wanna combine all LUT files into a single texture array considering they're all 256x256 and that'd save two texture slots
#ifdef AREA_LIGHTS
TEX_DECLARE2D(float4, LTC1, 3);
TEX_DECLARE2D(float4, LTC2, 4);
#endif
TEX_DECLARE2D(float, AmbientOcclusion, 5);
TEX_DECLARECUBE(float3, OutdoorCubemap, 6);
TEX_DECLARECUBEARRAY(float4, IndoorCubemaps, 7);
TEX_DECLARECUBEARRAY(float4, IrradianceCubemaps, 8);

/* Material Textures */
TEX_DECLARE2DARRAY(float4, Albedo,     10); /* R: Albedo.r           , G: Albedo.g         , B: Albedo.b         , A: Alpha/Factor      */
TEX_DECLARE2DARRAY(float4, Emissive,   11); /* R: Emissive.r         , G: Emissive.g       , B: Emissive.b       , A: Emissive Factor   */
TEX_DECLARE2DARRAY(float4, MatValues,  12); /* R: Roughness          , G: Metalness        , B: Ambient Occlusion, A: Unused            */

TEX_DECLARE2DARRAY(float3, NormalMap,  13); /* R: Normal.x           , G: Normal.y         , B: Normal.z         , A: Unused            */
TEX_DECLARE2DARRAY(float,  HeightMap,  14); /* R: Height             , G: -----------------, B: -----------------, A: ----------------- */

TEX_DECLARE2DARRAY(float4, ClearcoatA, 15); /* R: ClearcoatTint.r    , G: ClearcoatTint.g  , B: ClearcoatTint.b  , A: Clearcoat Factor  */
TEX_DECLARE2DARRAY(float4, ClearcoatB, 16); /* R: Clearcoat Roughness, G: ClearcoatNormal.x, B: ClearcoatNormal.y, A: ClearcoatNormal.z */
                                /* No Normals: R: Clearcoat Roughness, G: Unused           , B: -----------------, A: ----------------- */

#define MAX_REFLECTION_LOD 5.0f

float4 MaterialPS(MaterialVertexOutput IN) : SV_TARGET {
    int MaterialIndex = IN.MaterialID;

    float4 MaterialParametersA =      MaterialsData[MaterialIndex].RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused;
    int4   MaterialParametersB = int4(MaterialsData[MaterialIndex].ClearcoatEnabled_AlbedoMode_NormalMapEnabled_EmissiveMode);
    float4 MaterialParametersC =      MaterialsData[MaterialIndex].IndexOfRefraction_EmissiveFactor_ParallaxFactor_ParallaxOffset;

    float3 ViewDirection = normalize(CameraPosition.xyz - IN.WorldPosition.xyz);
    float3 UVW = float3(IN.UV, MaterialIndex);
    float IOR = MaterialParametersC.x;

    if (MaterialParametersC.z > 0.0f)
        UVW.xy = ParallaxOcclusionMapping(HeightMapTexture, AnisotropicWrap, UVW, ViewDirection, MaterialParametersC.z, MaterialParametersC.w);

    float4 Albedo = IN.Color;

    /* Transparency */
    if (MaterialParametersB.x == 1) {
        Albedo *= AlbedoTexture.Sample(AnisotropicWrap, UVW);
    }
    /* Overlay */
    else if (MaterialParametersB.x == 2) {
        float4 AlbedoMap = AlbedoTexture.Sample(AnisotropicWrap, UVW);

        Albedo.rgb = lerp(Albedo.rgb, AlbedoMap.rgb, AlbedoMap.a);
    }
    /* Tinting */
    else if (MaterialParametersB.x == 3) {
        float4 AlbedoMap = AlbedoTexture.Sample(AnisotropicWrap, UVW);

        Albedo.rgb = lerp(AlbedoMap.rgb, Albedo.rgb * AlbedoMap.rgb, AlbedoMap.a);
    }

    float Roughness = MaterialParametersA.y;
    float Metalness = MaterialParametersA.z;
    float LocalAO = 1.0f;

    if (all(MaterialParametersA.xy < 0.0f) && MaterialParametersA.z > 0.0f) {
        float3 MatValues = MatValuesTexture.Sample(AnisotropicWrap, UVW).xyz;

        // Ternary operators don't cause branching in HLSL
        Roughness = (MaterialParametersA.x < 0.0f) ? MatValues.x : Roughness;
        Metalness = (MaterialParametersA.y < 0.0f) ? MatValues.y : Metalness;
        LocalAO = (MaterialParametersA.z > 0.0f) ? mad(MatValues.z, MaterialParametersA.z, 1.0f - MaterialParametersA.z) : LocalAO;
    }

    Metalness = 1.0f - Metalness;

    float3 Normal = IN.Normal;

    if (MaterialParametersB.y == 1)
        Normal = normalize(mul(NormalMapTexture.Sample(AnisotropicWrap, UVW) * 2.0f - 1.0f, float3x3(IN.Tangent, IN.Bitangent, Normal)));

	float NDV = saturate(dot(Normal, ViewDirection));

    float AmbientDiffuseFactor = AmbientColor_EnvDiffuse.w * Metalness;
    float AmbientSpecularFactor = OutdoorAmbientColor_EnvSpecular.w;
    float OutdoorContribution = 1.0f; // This isn't going to work as a global variable, but it'll be like this until environment map instances are a thing.

    float3 AmbientContribution = OutdoorAmbientColor_EnvSpecular.rgb;

	if (AmbientDiffuseFactor > 0.0f) {
		float3 OutdoorDiffuse = IrradianceCubemapsTexture.Sample(LinearWrap, float4(Normal, 0.0f)).rgb;
		float4 IndoorDiffuse  = IrradianceCubemapsTexture.Sample(LinearWrap, float4(Normal, 1.0f));

        // The alpha channel in the indoor cubemap represents how visible the sky is.
        // By doing it this way, we avoid having to re-render the indoor cubemap when the sky changes.
        float SkylightContribution = max(OutdoorContribution, IndoorDiffuse.a);

		AmbientContribution += mad(OutdoorDiffuse, SkylightContribution, IndoorDiffuse.rgb * (1.0f - SkylightContribution)) * Albedo.rgb * AmbientDiffuseFactor;
	}

	if (AmbientSpecularFactor > 0.0f) {
		float2 EnvironmentBRDF = EnvironmentBRDFTexture.Sample(LinearWrap, float2(Roughness, NDV)).xy;
		float3 Fresnel = FresnelCombined(NDV, F0ToIOR(Albedo.rgb), float3(0.0f, 0.0f, 0.0f), IOR, Metalness);
        float3 Reflect = reflect(-ViewDirection, Normal);
		float3 BRDF = mad(Fresnel, EnvironmentBRDF.x, EnvironmentBRDF.y);

        float EnvRoughness = Roughness * MAX_REFLECTION_LOD;

		float3 OutdoorSpecular = OutdoorCubemapTexture.SampleLevel(LinearWrap, Reflect, EnvRoughness);
		float4 IndoorSpecular  = IndoorCubemapsTexture.SampleLevel(LinearWrap, float4(Reflect, 0.0f), EnvRoughness);

        float SkylightContribution = max(OutdoorContribution, IndoorSpecular.a);

		AmbientContribution += mad(OutdoorSpecular, SkylightContribution, IndoorSpecular.rgb * (1.0f - SkylightContribution)) * BRDF * AmbientSpecularFactor;
	}

    float3 TotalLight = float3(0.0f, 0.0f, 0.0f);

    if (any(KeyColor_KeyShadowDistance.rgb > TotalLight)) {
        float3 SunDirection = -KeyDirection_unused.xyz;

        TotalLight += Lighting(1.0f, 1.0f, Normal, SunDirection, ViewDirection, NDV, 1.0f, Albedo.rgb, Roughness, Metalness, IOR);
    }

    TotalLight += mad(AmbientContribution, LocalAO, AmbientColor_EnvDiffuse.rgb * (1.0f - LocalAO));

    return float4(TotalLight, Albedo.a);
}
