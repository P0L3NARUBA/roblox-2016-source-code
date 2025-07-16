#define MATERIAL
#include "default.hlsl"
#include "lighting.hlsli"

/* Main Textures */
TEX_DECLARE2D(ShadowAtlas, 0);
TEX_DECLARE2D(SunShadowArray, 1);
#ifdef AREA_LIGHTS
TEX_DECLARE2D(LTC1, 2);
TEX_DECLARE2D(LTC2, 3);
#endif
TEX_DECLARE2D(EnvironmentBRDF, 4);
TEX_DECLARE2D(AmbientOcclusion, 5);
TEX_DECLARECUBE(OutdoorCubemap, 6);
TEX_DECLARECUBE(IndoorCubemapA, 7);
TEX_DECLARECUBE(IndoorCubemapB, 8);

/* Material Textures */
TEX_DECLARE2DARRAY(Albedo, 10);      /* R: Albedo.r           , G: Albedo.g         , B: Albedo.b         , A: Alpha             */
TEX_DECLARE2DARRAY(MatValues, 11);   /* R: Roughness          , G: Metalness        , B: Ambient Occlusion, A: Height            */
TEX_DECLARE2DARRAY(NormalMap, 12);   /* R: Normal.x           , G: Normal.y         , B: Normal.z         , A: Unused            */
TEX_DECLARE2DARRAY(Emission, 13);    /* R: Emission.r         , G: Emission.g       , B: Emission.b       , A: Emission Factor   */
TEX_DECLARE2DARRAY(ClearcoatA, 14);  /* R: ClearcoatTint.r    , G: ClearcoatTint.g  , B: ClearcoatTint.b  , A: Clearcoat Factor  */
                                     /* R: Clearcoat Roughness, G: Clearcoat Factor , B: -----------------, A: ----------------- */
TEX_DECLARE2DARRAY(ClearcoatB, 15);  /* R: Clearcoat Roughness, G: ClearcoatNormal.x, B: ClearcoatNormal.y, A: ClearcoatNormal.z */
                         /* No Normals: R: Clearcoat Roughness, G: Unused           , B: -----------------, A: ----------------- */

float4 MaterialPS(MaterialVertexOutput IN) : SV_TARGET {
    int MaterialIndex = IN.MaterialID;
    float IOR = 1.46;

    float3 UVW = float3(IN.UV, MaterialIndex);

    float4 MaterialParametersA = float4(MaterialsData[MaterialIndex].AlbedoEnabled_RoughnessOverride_MetalnessOverride_EmissionOverride);
    int4 MaterialParametersB = int4(MaterialsData[MaterialIndex].ClearcoatEnabled_NormalMapEnabled_AmbientOcclusionEnabled_ParallaxEnabled);

    float4 Albedo = (int(MaterialParametersA.x) == 1) ? IN.Color * AlbedoTexture.Sample(AlbedoSampler, UVW) : IN.Color;

    float Roughness;
    float Metalness;
    float LocalAO;
    float Height;

    if (all(int2(MaterialParametersA.yz) == -1) && all(MaterialParametersB.zw == 1)) {
        float4 MatValues = MatValuesTexture.Sample(MatValuesSampler, UVW);

        // Ternary operators don't cause branching in HLSL
        Roughness = (int(MaterialParametersA.y) == -1) ? MatValues.x : MaterialParametersA.y;
        Metalness = 1.0 - ((int(MaterialParametersA.z) == -1) ? MatValues.y : MaterialParametersA.z);
        LocalAO = (int(MaterialParametersB.z) == 1) ? MatValues.z : 1.0;
        Height = MatValues.w; // If parallax is disabled, the code responsible for parallax won't run and thus this can be set to whatever
    }
    else {
        Roughness = MaterialParametersA.y;
        Metalness = 1.0 - MaterialParametersA.z;
        LocalAO = 1.0;
        Height = 0.0; // Same case as above
    }

    float3 Normal = (MaterialParametersB.y == 1) ? normalize(mul(float3x3(IN.Tangent, IN.Bitangent, IN.Normal), NormalMapTexture.Sample(NormalMapSampler, UVW).xyz * 2.0 - 1.0)) : IN.Normal;

    float3 ViewDirection = normalize(CameraPosition.xyz - IN.Position.xyz);
	float uNDV = dot(Normal, ViewDirection);
	float NDV = saturate(uNDV * 0.5 + 0.5); // Multiply-add is a single instruction so we do this instead of (+ 1.0) / 2.0

    float3 TotalLight = float3(0.0, 0.0, 0.0);

    if (any(KeyColor_KeyShadowDistance.rgb > TotalLight)) {
        float3 SunDirection = -KeyDirection_unused.xyz;

        TotalLight += Lighting(1.0, 1.0, Normal, SunDirection, ViewDirection, NDV, 1.0, Albedo.rgb, Roughness, Metalness, IOR);
    }

    return float4(TotalLight, Albedo.a);
}
