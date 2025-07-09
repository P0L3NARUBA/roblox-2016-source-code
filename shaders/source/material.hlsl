#define MATERIAL
#include "default.hlsl"
#include "lighting.hlsl"

/* Main Textures */
TEX_DECLARE2D(ShadowAtlas, 0);
#ifdef AREA_LIGHTS
TEX_DECLARE2D(LTC1, 1);
TEX_DECLARE2D(LTC2, 2);
#endif
TEX_DECLARE2D(EnvironmentBRDF, 3);
TEX_DECLARE2D(AmbientOcclusion, 4);
TEX_DECLARECUBE(OutdoorCubemap, 5);
TEX_DECLARECUBE(IndoorCubemapA, 6);
//TEX_DECLARECUBE(IndoorCubemapB, 7);

/* Material Textures */
TEX_DECLARE3D(Albedo, 10);      /* R: Albedo.r           , G: Albedo.g         , B: Albedo.b         , A: Alpha             */
TEX_DECLARE3D(MatValues, 11);   /* R: Roughness          , G: Metalness        , B: Ambient Occlusion, A: Height            */
TEX_DECLARE3D(NormalMap, 12);   /* R: Normal.x           , G: Normal.y         , B: Normal.z         , A: Unused            */
TEX_DECLARE3D(Emission, 13);    /* R: Emission.r         , G: Emission.g       , B: Emission.b       , A: Emission Factor   */
TEX_DECLARE3D(ClearcoatA, 14);  /* R: ClearcoatTint.r    , G: ClearcoatTint.g  , B: ClearcoatTint.b  , A: Clearcoat Factor  */
                                /* R: Clearcoat Roughness, G: Clearcoat Factor , B: -----------------, A: ----------------- */
TEX_DECLARE3D(ClearcoatB, 15);  /* R: Clearcoat Roughness, G: ClearcoatNormal.x, B: ClearcoatNormal.y, A: ClearcoatNormal.z */
                    /* No Normals: R: Clearcoat Roughness, G: Unused           , B: -----------------, A: ----------------- */

float4 MaterialPS(MaterialVertexOutput IN) : SV_TARGET {
    int MaterialIndex = IN.MaterialID;
    float IOR = 1.46;

    float3 UVW = float3(IN.UV, MaterialIndex);

    float4 MaterialParametersA = float4(MaterialsData[MaterialIndex].AlbedoEnabled_RoughnessOverride_MetalnessOverride_EmissionOverride);
    int4 MaterialParametersB = int4(MaterialsData[MaterialIndex].ClearcoatEnabled_NormalMapEnabled_AmbientOcclusionEnabled_ParallaxEnabled);

    float4 Albedo = (int(MaterialParametersA.x) == 1) ? IN.Color * AlbedoTexture.Sample(AlbedoSampler, UVW) : IN.Color;
    float4 MatValues = MatValuesTexture.Sample(MatValuesSampler, UVW);
    float Roughness = MatValues.x;
    float Metalness = 1.0 - MatValues.y;
    float LocalAO = MatValues.z;
    float Height = MatValues.w;

    float3 Normal = IN.Normal;

    if (MaterialParametersB.y == 1) {
        float3x3 TBN = float3x3(IN.Tangent, IN.Bitangent, Normal);

        Normal = normalize(mul(TBN, NormalMapTexture.Sample(NormalMapSampler, UVW).xyz * 2.0 - 1.0));
    }

    float3 ViewDirection = normalize(CameraPosition.xyz - IN.Position.xyz);
	float uNDV = dot(Normal, ViewDirection);
	float NDV = (clamp(uNDV, -1.0, 1.0) + 1.0) / 2.0;

    float3 TotalLight = float3(0.0, 0.0, 0.0);

    if (any(KeyColor_KeyShadowDistance.rgb > TotalLight)) {
        float3 SunDirection = -KeyDirection_unused.xyz;

        TotalLight += Lighting(1.0, 1.0, Normal, SunDirection, ViewDirection, NDV, 1.0, Albedo.rgb, Roughness, Metalness, IOR);
    }

    return float4(TotalLight, Albedo.a);
}
