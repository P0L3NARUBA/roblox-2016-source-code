#include "default.hlsl"

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
//TEX_DECLARECUBE(IndoorCubemapB, 6);

/* Material Textures */
TEX_DECLARE2D(Albedo, 10);      /* R: Albedo.r           , G: Albedo.g         , B: Albedo.b         , A: Alpha             */
TEX_DECLARE2D(MatValues, 11);   /* R: Roughness          , G: Metalness        , B: Ambient Occlusion, A: Height            */
TEX_DECLARE2D(NormalMap, 12);   /* R: Normal.x           , G: Normal.y         , B: Normal.z         , A: Unused            */
TEX_DECLARE2D(Emission, 13);    /* R: Emission.r         , G: Emission.g       , B: Emission.b       , A: Emission Factor   */
#ifdef CLEARCOAT_ADVANCED
TEX_DECLARE2D(CCTexA, 14);      /* R: ClearcoatTint.r    , G: ClearcoatTint.g  , B: ClearcoatTint.b  , A: Clearcoat Factor  */
TEX_DECLARE2D(CCTexB, 15);      /* R: Clearcoat Roughness, G: ClearcoatNormal.x, B: ClearcoatNormal.y, A: ClearcoatNormal.z */
                    /* No Normals: R: Clearcoat Roughness, G: Unused           , B: -----------------, A: ----------------- */
#elif CLEARCOAT_SIMPLE
TEX_DECLARE2D(CCTexA, 14);      /* R: Clearcoat Roughness, G: Clearcoat Factor , B: -----------------, A: ----------------- */
#endif

void MaterialPS(MaterialVertexOutput IN) : SV_TARGET {

}
