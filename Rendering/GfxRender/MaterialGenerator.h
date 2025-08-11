#pragma once

#include "g3d/Vector4.h"

#include "TextureCompositor.h"
#include "GlobalShaderData.h"

#include "v8datamodel/PartInstance.h"

namespace RBX {
	class Decal;
	class Humanoid;
	class HumanoidIdentifier;
}

namespace RBX {
	namespace Graphics {

		static const uint32_t kMatCacheSize = 256u;

		class VisualEngine;
		class Material;
		class TextureRef;
		class Technique;

		class MaterialGenerator {
		public:
			enum MaterialFlags {
				Flag_Skinned = 1 << 0,
				Flag_Transparent = 1 << 1,
				Flag_AlphaKill = 1 << 2,
				Flag_ForceDecal = 1 << 3,
				Flag_ForceDecalTexture = 1 << 4,
				Flag_UseCompositTexture = 1 << 5,

				Flag_CacheMask = (Flag_Skinned | Flag_Transparent | Flag_AlphaKill | Flag_ForceDecal | Flag_ForceDecalTexture) + 1u,
			};

			enum ResultFlags {
				Result_UsesTexture = 1u << 0u,
				Result_UsesCompositTexture = 1u << 1u,
			};

			struct Result {
				explicit Result(const shared_ptr<Material>& material = shared_ptr<Material>(), uint32_t key = 0u)
					: material(material)
					, key(key)
				{
				}

				shared_ptr<Material> material;
				uint32_t key;
			};

			MaterialGenerator(VisualEngine* visualEngine);

			Result createMaterial(PartInstance* part, Decal* decal, uint32_t flags);

			void garbageCollectIncremental();
			void garbageCollectFull();

			static MaterialData getParameters(PartMaterial material);
			static float getTiling(PartMaterial material);
			static uint32_t getMaterialId(PartMaterial material);

			GlobalMaterialData getMaterialData() { return globalMaterialData; }

			static uint32_t createFlags(RBX::PartInstance* part, bool skinned);

		private:
			VisualEngine* visualEngine;

			typedef boost::unordered_map<std::string, shared_ptr<Material>> TexturedMaterialMap;

			struct TexturedMaterialCache {
				TexturedMaterialMap map;

				size_t gcSizeLast;
				std::string gcKeyNext;

				TexturedMaterialCache();
			};

			shared_ptr<Material> renderMaterialCache[2];
			TexturedMaterialCache texturedMaterialCache[Flag_CacheMask]; // an entry for each supported flag combination

			std::pair<Humanoid*, TextureCompositor::JobHandle> compositCache;

			TextureRef albedoTextures;	 /* R: Albedo.r,   G: Albedo.g,   B: Albedo.b,			A: Alpha/Factor	*/
			TextureRef emissiveTextures; /* R: Emissive.r, G: Emissive.g, B: Emissive.b,		A: Factor		*/
			TextureRef matValueTextures; /* R: Roughness,  G: Metalness,  B: Ambient Occlusion,	A: Unused		*/

			TextureRef normalTextures; /* R: Normal.x, G: Normal.y, B: Normal.z, A: Unused */
			TextureRef heightTextures; /* R: Height,   G: --------, B: --------, A: ------ */
			// We have the height texture separate rather than pack it with another texture, like the material value or normal map textures.
			// This is to avoid excessive memory bandwith use because it's sampled multiple times with our parallax occlusion mapping system.

			TextureRef clearcoatATextures; /* R: ClearcoatTint.r,     G: ClearcoatTint.g,   B: ClearcoatTint.b,   A: Clearcoat Factor  */
			TextureRef clearcoatBTextures; /* R: Clearcoat Roughness, G: ClearcoatNormal.x, B: ClearcoatNormal.y, A: ClearcoatNormal.z */

			GlobalMaterialData globalMaterialData;

			void setupComplexMaterialTextures(VisualEngine* visualEngine, Technique& technique, const std::string& materialName, uint32_t materialId, const std::string& materialVariant);
			void setupMaterialTextures(VisualEngine* ve, Technique& technique, PartMaterial renderMaterial);
			void assignMaterialTextures(VisualEngine* visualEngine, Technique& technique) const;

			void createTextureArrays(VisualEngine* visualEngine);

			Result createRenderMaterial(uint32_t flags, PartMaterial renderMaterial);
			Result createMaterialForPart(PartInstance* part, uint32_t flags);
			Result createMaterialForDecal(Decal* decal, uint32_t flags);

			//TextureRef  wangTilesTex;
		};

	}
}
