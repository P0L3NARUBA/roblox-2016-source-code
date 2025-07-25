#define MATERIAL
#include "default.hlsl"
#include "lighting.hlsli"
#include "parallax.hlsli"

/* Main Textures */
TEX_DECLARE2D(float4, ShadowAtlas, 0);
TEX_DECLARE2D(float4, SunShadowArray, 1);
TEX_DECLARE2D(float2, EnvironmentBRDF, 2); // may wanna combine all LUT files into a single texture array considering they're all 256x256 and that'd save two texture/sampler slots
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

#define MAX_REFLECTION_LOD 5.0

float4 MaterialPS(MaterialVertexOutput IN) : SV_TARGET {
    int MaterialIndex = IN.MaterialID;

    float4 MaterialParametersA =      MaterialsData[MaterialIndex].RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused;
    int4   MaterialParametersB = int4(MaterialsData[MaterialIndex].ClearcoatEnabled_AlbedoMode_NormalMapEnabled_EmissiveMode);
    float4 MaterialParametersC =      MaterialsData[MaterialIndex].IndexOfRefraction_EmissiveFactor_ParallaxFactor_ParallaxOffset;

    float3 ViewDirection = normalize(CameraPosition.xyz - IN.Position.xyz);
    float3 UVW = float3(IN.UV, MaterialIndex);
    float IOR = MaterialParametersC.x;

    /*if (MaterialParametersC.z > 0.0) {
        UVW.xy = ParallaxOcclusionMapping(HeightMapTexture, HeightMapSampler, UVW, ViewDirection, MaterialParametersC.z, MaterialParametersC.w);
    }*/

    float4 Albedo = IN.Color;

    if (MaterialParametersB.x == 1) {
        Albedo *= AlbedoTexture.Sample(AlbedoSampler, UVW);
    }
    else if (MaterialParametersB.x == 2) {
        float4 AlbedoMap = AlbedoTexture.Sample(AlbedoSampler, UVW);

        Albedo.rgb = lerp(Albedo.rgb, AlbedoMap.rgb, AlbedoMap.a);
    }
    else if (MaterialParametersB.x == 3) {
        float4 AlbedoMap = AlbedoTexture.Sample(AlbedoSampler, UVW);

        Albedo.rgb = lerp(AlbedoMap.rgb, Albedo.rgb * AlbedoMap.rgb, AlbedoMap.a);
    }

    float Roughness;
    float Metalness;
    float LocalAO;

    if (all(MaterialParametersA.xy < 0.0) && MaterialParametersA.z > 0.0) {
        float3 MatValues = MatValuesTexture.Sample(MatValuesSampler, UVW).xyz;

        // Ternary operators don't cause branching in HLSL
        Roughness = (MaterialParametersA.x < 0.0) ? MatValues.x : MaterialParametersA.x;
        Metalness = (MaterialParametersA.y < 0.0) ? MatValues.y : MaterialParametersA.y;
        LocalAO = (MaterialParametersA.z > 0.0) ? MatValues.z * MaterialParametersA.z + (1.0 - MaterialParametersA.z) : 1.0;
    }
    else {
        Roughness = MaterialParametersA.y;
        Metalness = MaterialParametersA.z;
        LocalAO = 1.0;
    }

    Metalness = 1.0 - Metalness;

    float3 Normal = (MaterialParametersB.y == 1) ? normalize(mul(float3x3(IN.Tangent, IN.Bitangent, IN.Normal), NormalMapTexture.Sample(NormalMapSampler, UVW) * 2.0 - 1.0)) : IN.Normal;
	float uNDV = dot(Normal, ViewDirection);
	float NDV = saturate(uNDV * 0.5 + 0.5); // Multiply-add is a single instruction so we do this instead of (+ 1.0) / 2.0
    float3 Reflect = reflect(-ViewDirection, Normal);

    float AmbientDiffuseFactor = AmbientColor_EnvDiffuse.w * Metalness;
    float AmbientSpecularFactor = OutdoorAmbientColor_EnvSpecular.w;
    float OutdoorContribution = 1.0; // Not sure how this is going to work with indoor/outdoor cubemapping. Manual indoor defining, maybe?

    float3 AmbientContribution = OutdoorAmbientColor_EnvSpecular.rgb;

	if (AmbientDiffuseFactor + AmbientSpecularFactor > 0.0) {
		if (AmbientDiffuseFactor > 0.0) {
			float3 OutdoorDiffuse = IrradianceCubemapsTexture.Sample(IrradianceCubemapsSampler, float4(Normal, 0.0));
			float4 IndoorDiffuse  = IrradianceCubemapsTexture.Sample(IrradianceCubemapsSampler, float4(Normal, 1.0));

            // The alpha channel in the indoor cubemap represents how visible the sky is.
            // By doing it this way, we avoid having to re-render the indoor cubemap when the sky changes.
            float SkylightContribution = max(OutdoorContribution, IndoorDiffuse.a);

			AmbientContribution += (OutdoorDiffuse * SkylightContribution + IndoorDiffuse.rgb * (1.0 - SkylightContribution)) * Albedo.rgb * AmbientDiffuseFactor;
		}

		if (AmbientSpecularFactor > 0.0) {
			float2 envBRDF = EnvironmentBRDFTexture.Sample(EnvironmentBRDFSampler, float2(Roughness, saturate(uNDV))).xy;
		    float3 F = Fresnel(NDV, F0ToIOR(Albedo.rgb), float3(0.0, 0.0, 0.0), IOR, Metalness);
			float3 BRDF = (F * envBRDF.x + envBRDF.y);

            float EnvRoughness = Roughness * MAX_REFLECTION_LOD;

			float3 OutdoorSpecular = OutdoorCubemapTexture.SampleLevel(OutdoorCubemapSampler, Reflect, EnvRoughness);
			float4 IndoorSpecular  = IndoorCubemapsTexture.SampleLevel(IndoorCubemapsSampler, float4(Reflect, 0.0), EnvRoughness);

            float SkylightContribution = max(OutdoorContribution, IndoorSpecular.a);

			AmbientContribution += (OutdoorSpecular * SkylightContribution + IndoorSpecular.rgb * (1.0 - SkylightContribution)) * BRDF * AmbientSpecularFactor;
		}
	}

    float3 TotalLight = float3(0.0, 0.0, 0.0);

    if (any(KeyColor_KeyShadowDistance.rgb > TotalLight)) {
        float3 SunDirection = -KeyDirection_unused.xyz;

        TotalLight += Lighting(1.0, 1.0, Normal, SunDirection, ViewDirection, NDV, 1.0, Albedo.rgb, Roughness, Metalness, IOR);
    }

    TotalLight += AmbientContribution * LocalAO + AmbientColor_EnvDiffuse.rgb * (1.0 - LocalAO);

    return float4(TotalLight, Albedo.a);
}
