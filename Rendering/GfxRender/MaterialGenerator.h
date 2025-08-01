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

		static const uint32_t kMatCacheSize = 64u;

		class VisualEngine;
		class Material;
		class TextureRef;

		class MaterialGenerator
		{
		public:
			enum MaterialFlags {
				Flag_Skinned = 1 << 0,
				Flag_Transparent = 1 << 1,
				Flag_AlphaKill = 1 << 2,
				Flag_ForceDecal = 1 << 3,
				Flag_ForceDecalTexture = 1 << 4,
				Flag_UseCompositTexture = 1 << 5,
				Flag_UseCompositTextureForAccoutrements = 1 << 6,
				Flag_DisableMaterialsAndStuds = 1 << 7,

				Flag_CacheMask = Flag_Skinned | Flag_Transparent | Flag_AlphaKill | Flag_ForceDecal | Flag_ForceDecalTexture
			};

			enum ResultFlags {
				Result_UsesTexture = 1 << 0,
				Result_UsesCompositTexture = 1 << 1,
				Result_PlasticLOD = 1 << 2,
			};

			struct Result {
				explicit Result(const shared_ptr<Material>& material = shared_ptr<Material>(), uint32_t flags = 0u, uint32_t features = 0u, const G3D::Vector4& uvOffsetScale = G3D::Vector4(0.0f, 0.0f, 1.0f, 1.0f))
					: material(material)
					, flags(flags)
					, features(features)
					, uvOffsetScale(uvOffsetScale)
				{

				}

				shared_ptr<Material> material;
				uint32_t flags;
				uint32_t features;
				G3D::Vector4 uvOffsetScale;
			};

			MaterialGenerator(VisualEngine* visualEngine);

			Result createMaterial(PartInstance* part, Decal* decal, const HumanoidIdentifier* hi, uint32_t flags);

			void invalidateCompositCache();

			void garbageCollectIncremental();
			void garbageCollectFull();

			static MaterialData getParameters(PartMaterial material);
			static float getTiling(PartMaterial material);
			static uint32_t getMaterialId(PartMaterial material);

			GlobalMaterialData getMaterialData() { return globalMaterialData; }

			static uint32_t createFlags(bool skinned, RBX::PartInstance* part, const HumanoidIdentifier* humanoidIdentifier, bool& ignoreDecalsOut);

			static uint32_t getDiffuseMapStage();

		private:
			VisualEngine* visualEngine;

			typedef boost::unordered_map<std::string, shared_ptr<Material> > TexturedMaterialMap;

			struct TexturedMaterialCache {
				TexturedMaterialMap map;

				size_t gcSizeLast;
				std::string gcKeyNext;

				TexturedMaterialCache();
			};

			shared_ptr<Material> baseMaterialCache[Flag_CacheMask + 1]; // an entry for each supported flag combination
			shared_ptr<Material> renderMaterialCache[kMatCacheSize][Flag_CacheMask + 1]; // an entry for each render material and each supported flag combination
			TexturedMaterialCache texturedMaterialCache[Flag_CacheMask + 1]; // an entry for each supported flag combination

			std::pair<Humanoid*, TextureCompositor::JobHandle> compositCache;

			TextureRef albedoTextures;	 /* R: Albedo.r,   G: Albedo.g,   B: Albedo.b,			A: Alpha/Factor	*/
			TextureRef emissiveTextures; /* R: Emissive.r, G: Emissive.g, B: Emissive.b,		A: Factor		*/
			TextureRef matValueTextures; /* R: Roughness,  G: Metalness,  B: Ambient Occlusion,	A: Unused		*/

			TextureRef normalTextures; /* R: Normal.x, G: Normal.y, B: Normal.z, A: Unused */
			TextureRef heightTextures; /* R: Height,   G: --------, B: --------, A: ------ */
			// We have the height texture separate rather than pack it with another texture, like the normal map texture.
			// This is to avoid excessive memory bandwith use because it's sampled multiple times with our parallax occlusion mapping system.

			TextureRef clearcoatATextures; /* R: ClearcoatTint.r,     G: ClearcoatTint.g,   B: ClearcoatTint.b,   A: Clearcoat Factor  */
			TextureRef clearcoatBTextures; /* R: Clearcoat Roughness, G: ClearcoatNormal.x, B: ClearcoatNormal.y, A: ClearcoatNormal.z */

			GlobalMaterialData globalMaterialData;

			void setupSmoothPlasticTextures(VisualEngine* visualEngine, Technique& technique);
			void setupComplexMaterialTextures(VisualEngine* visualEngine, Technique& technique, const std::string& materialName, uint32_t materialId, const std::string& materialVariant);
			void setupMaterialTextures(VisualEngine* ve, Technique& technique, PartMaterial renderMaterial);
			void assignMaterialTextures(VisualEngine* visualEngine, Technique& technique) const;

			void createTextureArrays(VisualEngine* visualEngine);

			shared_ptr<Material> createBaseMaterial(uint32_t flags);
			shared_ptr<Material> createRenderMaterial(uint32_t flags, PartMaterial renderMaterial);
			shared_ptr<Material> createTexturedMaterial(const TextureRef& texture, const std::string& textureName, uint32_t flags);

			Result createDefaultMaterial(PartInstance* part, uint32_t flags, PartMaterial renderMaterial);
			Result createMaterialForPart(PartInstance* part, const HumanoidIdentifier* hi, uint32_t flags);
			Result createMaterialForDecal(Decal* decal, uint32_t flags);

			//TextureRef  wangTilesTex;
		};

	}
}
