#include "stdafx.h"
#include "MaterialGenerator.h"

#include "GfxBase/PartIdentifier.h"
//#include "GfxBase/GfxPart.h"

#include "TextureCompositor.h"
#include "TextureManager.h"
#include "VisualEngine.h"

#include "v8datamodel/PartInstance.h"
#include "v8datamodel/CharacterMesh.h"
#include "v8datamodel/Decal.h"
#include "v8datamodel/SpecialMesh.h"
//#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/Accoutrement.h"
//#include "v8datamodel/BlockMesh.h"
//#include "v8datamodel/CylinderMesh.h"
#include "v8datamodel/PartCookie.h"


#include "Material.h"
#include "ShaderManager.h"
//#include "TextureManager.h"
#include "util/SafeToLower.h"

#include "GfxCore/Device.h"
#include "SceneManager.h"
#include "EnvMap.h"

#include "RenderQueue.h"

FASTFLAGVARIABLE(RenderMaterialsOnMobile, true)
FASTFLAGVARIABLE(ForceWangTiles, false)
//FASTFLAG(GlowEnabled)

namespace RBX {
	namespace Graphics {

		static const char* getMaterialName(PartMaterial material) {
			switch (material) {
			case ASPHALT_MATERIAL:			return "Asphalt";
			case BASALT_MATERIAL:			return "Basalt";
			case BRICK_MATERIAL:			return "Brick";
			case CARDBOARD_MATERIAL:		return "Cardboard";
			case CARPET_MATERIAL:			return "Carpet";
			case CERAMIC_TILES_MATERIAL:	return "Ceramic Tiles";
			case CLAY_ROOF_TILES_MATERIAL:	return "Clay Roof Tiles";
			case COBBLESTONE_MATERIAL:		return "Cobblestone";
			case CONCRETE_MATERIAL:			return "Concrete";
			case CRACKED_LAVA_MATERIAL:		return "Cracked Lava";
			case DIAMONDPLATE_MATERIAL:		return "Diamond Plate";
			case FABRIC_MATERIAL:			return "Fabric";
			case FOIL_MATERIAL:				return "Foil";
			case GLACIER_MATERIAL:			return "Glacier";
			case GRANITE_MATERIAL:			return "Granite";
			case GRASS_MATERIAL:			return "Grass";
			case GROUND_MATERIAL:			return "Ground";
			case ICE_MATERIAL:				return "Ice";
			case LEAFY_GRASS_MATERIAL:		return "Leafy Grass";
			case LEATHER_MATERIAL:			return "Leather";
			case LIMESTONE_MATERIAL:		return "Limestone";
			case MARBLE_MATERIAL:			return "Marble";
			case METAL_MATERIAL:			return "Metal";
			case MUD_MATERIAL:				return "Mud";
			case NEON_MATERIAL:				return "Neon";
			case PAVEMENT_MATERIAL:			return "Pavement";
			case PEBBLE_MATERIAL:			return "Pebble";
			case PLASTER_MATERIAL:			return "Plaster";
			case PLASTIC_MATERIAL:			return "Plastic";
			case ROCK_MATERIAL:				return "Rock";
			case ROOF_SHINGLES_MATERIAL:	return "Roof Shingles";
			case RUBBER_MATERIAL:			return "Rubber";
			case RUST_MATERIAL:				return "Rust";
			case SALT_MATERIAL:				return "Salt";
			case SAND_MATERIAL:				return "Sand";
			case SANDSTONE_MATERIAL:		return "Sandstone";
			case SLATE_MATERIAL:			return "Slate";
			case SMOOTH_PLASTIC_MATERIAL:	return "Smooth Plastic";
			case SNOW_MATERIAL:				return "Snow";
			case WOOD_MATERIAL:				return "Wood";
			case WOODPLANKS_MATERIAL:		return "Wood Planks";

			default:
				RBXASSERT(false);

				return "Smooth Plastic";
			}
		};

		uint32_t MaterialGenerator::getMaterialId(PartMaterial material) {
			switch (material) {
			case ASPHALT_MATERIAL:			return 1u;
			case BASALT_MATERIAL:			return 2u;
			case BRICK_MATERIAL:			return 3u;
			case CARDBOARD_MATERIAL:		return 4u;
			case CARPET_MATERIAL:			return 5u;
			case CERAMIC_TILES_MATERIAL:	return 6u;
			case CLAY_ROOF_TILES_MATERIAL:	return 7u;
			case COBBLESTONE_MATERIAL:		return 8u;
			case CONCRETE_MATERIAL:			return 9u;
			case CRACKED_LAVA_MATERIAL:		return 10u;
			case DIAMONDPLATE_MATERIAL:		return 11u;
			case FABRIC_MATERIAL:			return 12u;
			case FOIL_MATERIAL:				return 13u;
			case GLACIER_MATERIAL:			return 14u;
			case GRANITE_MATERIAL:			return 15u;
			case GRASS_MATERIAL:			return 16u;
			case GROUND_MATERIAL:			return 17u;
			case ICE_MATERIAL:				return 18u;
			case LEAFY_GRASS_MATERIAL:		return 19u;
			case LEATHER_MATERIAL:			return 20u;
			case LIMESTONE_MATERIAL:		return 21u;
			case MARBLE_MATERIAL:			return 22u;
			case METAL_MATERIAL:			return 23u;
			case MUD_MATERIAL:				return 24u;
			case PAVEMENT_MATERIAL:			return 25u;
			case PEBBLE_MATERIAL:			return 26u;
			case PLASTER_MATERIAL:			return 27u;
			case PLASTIC_MATERIAL:			return 28u;
			case ROCK_MATERIAL:				return 29u;
			case ROOF_SHINGLES_MATERIAL:	return 30u;
			case RUBBER_MATERIAL:			return 31u;
			case RUST_MATERIAL:				return 32u;
			case SALT_MATERIAL:				return 33u;
			case SAND_MATERIAL:				return 34u;
			case SANDSTONE_MATERIAL:		return 35u;
			case SLATE_MATERIAL:			return 36u;
			case SNOW_MATERIAL:				return 37u;
			case WOOD_MATERIAL:				return 38u;
			case WOODPLANKS_MATERIAL:		return 39u;

			default:						return 0u;
			}
		}

		static PartMaterial getMaterialFromId(uint32_t materialId) {
			switch (materialId) {
			case 1u:  return ASPHALT_MATERIAL;
			case 2u:  return BASALT_MATERIAL;
			case 3u:  return BRICK_MATERIAL;
			case 4u:  return CARDBOARD_MATERIAL;
			case 5u:  return CARPET_MATERIAL;
			case 6u:  return CERAMIC_TILES_MATERIAL;
			case 7u:  return CLAY_ROOF_TILES_MATERIAL;
			case 8u:  return COBBLESTONE_MATERIAL;
			case 9u:  return CONCRETE_MATERIAL;
			case 10u: return CRACKED_LAVA_MATERIAL;
			case 11u: return DIAMONDPLATE_MATERIAL;
			case 12u: return FABRIC_MATERIAL;
			case 13u: return FOIL_MATERIAL;
			case 14u: return GLACIER_MATERIAL;
			case 15u: return GRANITE_MATERIAL;
			case 16u: return GRASS_MATERIAL;
			case 17u: return GROUND_MATERIAL;
			case 18u: return ICE_MATERIAL;
			case 19u: return LEAFY_GRASS_MATERIAL;
			case 20u: return LEATHER_MATERIAL;
			case 21u: return LIMESTONE_MATERIAL;
			case 22u: return MARBLE_MATERIAL;
			case 23u: return METAL_MATERIAL;
			case 24u: return MUD_MATERIAL;
			case 25u: return PAVEMENT_MATERIAL;
			case 26u: return PEBBLE_MATERIAL;
			case 27u: return PLASTER_MATERIAL;
			case 28u: return PLASTIC_MATERIAL;
			case 29u: return ROCK_MATERIAL;
			case 30u: return ROOF_SHINGLES_MATERIAL;
			case 31u: return RUBBER_MATERIAL;
			case 32u: return RUST_MATERIAL;
			case 33u: return SALT_MATERIAL;
			case 34u: return SAND_MATERIAL;
			case 35u: return SANDSTONE_MATERIAL;
			case 36u: return SLATE_MATERIAL;
			case 37u: return SNOW_MATERIAL;
			case 38u: return WOOD_MATERIAL;
			case 39u: return WOODPLANKS_MATERIAL;

			default:  return SMOOTH_PLASTIC_MATERIAL;
			}
		}

#ifdef RBX_PLATFORM_IOS
		static const std::string kTextureExtension = ".pvr";
#elif defined(__ANDROID__)
		static const std::string kTextureExtension = ".pvr";
#else
		static const std::string kTextureExtension = ".dds";
#endif

		static void setupShadowDepthTechnique(Technique& technique) {
			technique.setRasterizerState(RasterizerState::Cull_Back);
		}

		static void setupTechnique(Technique& technique, uint32_t flags) {
			if (flags & MaterialGenerator::Flag_Transparent) {
				technique.setBlendState(BlendState::Mode_AlphaBlend);

				technique.setDepthState(DepthState(DepthState::Function_GreaterEqual, false));
			}
			else {
				technique.setBlendState(BlendState::Mode_None);

				technique.setDepthState(DepthState(DepthState::Function_GreaterEqual, true));
			}

			technique.setRasterizerState(RasterizerState(RasterizerState::Cull_Back, 0u));
		}

		static void copyTextureToArray(Texture* targetTexture, Texture* sourceTexture, uint32_t materialId = 0u) {
			for (uint32_t i = 0u; i < sourceTexture->getMipLevels(); ++i) {
				void* data = nullptr;

				sourceTexture->download(0u, i, data, 0u);

				targetTexture->upload(materialId, i, TextureRegion(0u, 0u, 1024u, 1024u), data, sizeof(data));
			}

			targetTexture->commitChanges();
		}

		void MaterialGenerator::setupComplexMaterialTextures(VisualEngine* visualEngine, Technique& technique, const std::string& materialName, uint32_t materialId, const std::string& materialVariant) {
			TextureManager* tm = visualEngine->getTextureManager();

			std::string texturePath = "rbxasset://textures/" + materialVariant + "/" + materialName + "/";

			safeToLower(texturePath);
			texturePath.erase(remove(texturePath.begin(), texturePath.end(), ' '), texturePath.end());

			//copyTextureToArray(albedoTextures.getTexture().get(), tm->load(ContentId(texturePath + "color" + kTextureExtension), TextureManager::Fallback_White).getTexture().get(), materialId);
		}

		void MaterialGenerator::assignMaterialTextures(VisualEngine* visualEngine, Technique& technique) const {
			technique.setTexture(10u, albedoTextures);
			technique.setTexture(11u, emissiveTextures);
			technique.setTexture(12u, matValueTextures);

			technique.setTexture(13u, normalTextures);
			technique.setTexture(14u, heightTextures);

			technique.setTexture(15u, clearcoatATextures);
			technique.setTexture(16u, clearcoatBTextures);
		}

		static void setupCommonTextures(VisualEngine* visualEngine, Technique& technique) {
			TextureManager* tm = visualEngine->getTextureManager();
			SceneManager* sceneManager = visualEngine->getSceneManager();

			/* Shadow Maps */
			technique.setTexture(0u, sceneManager->getShadowMapAtlas());
			technique.setTexture(1u, sceneManager->getShadowMapArray());

			/* Environment BRDF */
			technique.setTexture(2u, tm->load(ContentId("rbxasset://textures/brdfLUT" + kTextureExtension), TextureManager::Fallback_None).getTexture());

			/* Area Light LTCs */
			technique.setTexture(3u, tm->load(ContentId("rbxasset://textures/ltc1LUT" + kTextureExtension), TextureManager::Fallback_None).getTexture());
			technique.setTexture(4u, tm->load(ContentId("rbxasset://textures/ltc2LUT" + kTextureExtension), TextureManager::Fallback_None).getTexture());

			/* Ambient Occlusion */
			technique.setTexture(5u, sceneManager->getAmbientOcclusion());

			/* Cubemaps */
			technique.setTexture(6u, sceneManager->getEnvMap()->getOutdoorTexture());
			technique.setTexture(7u, sceneManager->getEnvMap()->getIndoorTextures());
			technique.setTexture(8u, sceneManager->getEnvMap()->getIrradianceTextures());
		}

		void MaterialGenerator::setupMaterialTextures(VisualEngine* ve, Technique& technique, PartMaterial renderMaterial) {
			if (renderMaterial == PLASTIC_MATERIAL)
				setupComplexMaterialTextures(ve, technique, getMaterialName(renderMaterial), getMaterialId(renderMaterial), "global");
			else
				setupComplexMaterialTextures(ve, technique, getMaterialName(renderMaterial), getMaterialId(renderMaterial), "modern");
		}

		void MaterialGenerator::createTextureArrays(VisualEngine* visualEngine) {
			if (!albedoTextures) {
				Device* device = visualEngine->getDevice();

				albedoTextures = device->createTexture(Texture::Type_2D_Array, Texture::Format_BC7_sRGB, 1024u, 1024u, 48u, 11u, Texture::Usage_Static);
				emissiveTextures = device->createTexture(Texture::Type_2D_Array, Texture::Format_BC7_sRGB, 1024u, 1024u, 48u, 11u, Texture::Usage_Static);
				matValueTextures = device->createTexture(Texture::Type_2D_Array, Texture::Format_BC7, 1024u, 1024u, 48u, 11u, Texture::Usage_Static);

				normalTextures = device->createTexture(Texture::Type_2D_Array, Texture::Format_BC7, 1024u, 1024u, 48u, 11u, Texture::Usage_Static);
				heightTextures = device->createTexture(Texture::Type_2D_Array, Texture::Format_BC4, 512u, 512u, 48u, 10u, Texture::Usage_Static);

				clearcoatATextures = device->createTexture(Texture::Type_2D_Array, Texture::Format_BC7_sRGB, 1024u, 1024u, 48u, 11u, Texture::Usage_Static);
				clearcoatBTextures = device->createTexture(Texture::Type_2D_Array, Texture::Format_BC7, 1024u, 1024u, 48u, 11u, Texture::Usage_Static);

				for (uint32_t i = 0u; i == 39u; ++i)
					globalMaterialData.Materials[i] = getParameters(getMaterialFromId(i));
			}
		}

		MaterialGenerator::TexturedMaterialCache::TexturedMaterialCache()
			: gcSizeLast(0u)
		{
		}

		MaterialGenerator::MaterialGenerator(VisualEngine* visualEngine)
			: visualEngine(visualEngine)
			, compositCache(nullptr, TextureCompositor::JobHandle())
		{
			createTextureArrays(visualEngine);
		}

		MaterialGenerator::Result MaterialGenerator::createRenderMaterial(uint32_t flags, PartMaterial renderMaterial) {
			uint32_t materialId = getMaterialId(renderMaterial);

			uint32_t cacheKey = renderMaterial == NEON_MATERIAL ? 0u : 1u;

			// Fast cache lookup
			if (renderMaterialCache[cacheKey])
				return MaterialGenerator::Result(renderMaterialCache[cacheKey], cacheKey);

			// Create material
			std::shared_ptr<Material> material(new Material());

			TextureManager* tm = visualEngine->getTextureManager();

			//std::string vertexSkinning = (flags & Flag_Skinned) ? "Skinned" : "Static";

			if (renderMaterial == NEON_MATERIAL) {
				if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("BasicMaterialVS", "NeonFS")) {
					Technique technique(program, 0u);

					setupTechnique(technique, flags);

					material->addTechnique(technique);
				}
			}
			else {
				if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("MaterialVS", "RegularLOD0FS")) {
					Technique technique(program, 0u);

					setupTechnique(technique, flags);

					setupCommonTextures(visualEngine, technique);
					setupMaterialTextures(visualEngine, technique, renderMaterial);
					assignMaterialTextures(visualEngine, technique);

					material->addTechnique(technique);
				}
				else if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("BasicMaterialVS", "FallbackFS")) {
					Technique technique(program, 0u);

					setupTechnique(technique, flags);

					material->addTechnique(technique);
				}
			}

			// Fast cache fill
			renderMaterialCache[cacheKey] = material;

			return MaterialGenerator::Result(material, cacheKey);
		}

		MaterialGenerator::Result MaterialGenerator::createMaterialForPart(PartInstance* part, uint32_t flags) {
			DataModelMesh* specialShape = getSpecialShape(part);

			/*if (FileMesh* fileMesh = getFileMesh(specialShape)) {
				const TextureId& textureId = fileMesh->getTextureId();

				TextureRef texture = textureId.isNull() ? TextureRef() : visualEngine->getTextureManager()->load(textureId, TextureManager::Fallback_Gray, fileMesh->getFullName() + ".TextureId");

				return texture.getTexture()
					? Result(createTexturedMaterial(texture, textureId.toString(), flags), Result_UsesTexture)
					: createDefaultMaterial(part, flags, SMOOTH_PLASTIC_MATERIAL);
			}*/

			return createRenderMaterial(flags, part->getRenderMaterial());
		}

		MaterialGenerator::Result MaterialGenerator::createMaterialForDecal(Decal* decal, uint32_t flags) {
			/*const TextureId& textureId = decal->getTexture();

			TextureRef texture = textureId.isNull() ? TextureRef() : visualEngine->getTextureManager()->load(textureId, TextureManager::Fallback_BlackTransparent, decal->getFullName() + ".Texture");

			return texture.getTexture()
				? Result(createTexturedMaterial(texture, textureId.toString(), flags), Result_UsesTexture)
				: Result();*/
			return MaterialGenerator::Result();
		}

		MaterialGenerator::Result MaterialGenerator::createMaterial(PartInstance* part, Decal* decal, uint32_t flags) {
			if (decal) {
				if (decal->isA<DecalTexture>())
					return createMaterialForDecal(decal, flags | Flag_ForceDecalTexture);
				else
					return createMaterialForDecal(decal, flags | Flag_ForceDecal);
			}
			else
				return createMaterialForPart(part, flags);
		}

		void MaterialGenerator::garbageCollectIncremental() {
			for (size_t i = 0u; i < ARRAYSIZE(texturedMaterialCache); ++i) {
				TexturedMaterialCache& cache = texturedMaterialCache[i];

				// To catch up with allocation rate we need to visit the number of allocated elements since last run plus a small constant
				size_t visitCount = std::min(cache.map.size(), std::max(cache.map.size(), cache.gcSizeLast) - cache.gcSizeLast + 8);

				TexturedMaterialMap::iterator it = cache.map.find(cache.gcKeyNext);

				for (size_t j = 0u; j < visitCount; ++j) {
					if (it == cache.map.end())
						it = cache.map.begin();

					if (it->second.unique())
						it = cache.map.erase(it);
					else
						++it;
				}

				cache.gcSizeLast = cache.map.size();
				cache.gcKeyNext = (it == cache.map.end()) ? "" : it->first;
			}
		}

		void MaterialGenerator::garbageCollectFull() {
			for (size_t i = 0u; i < ARRAYSIZE(texturedMaterialCache); ++i) {
				TexturedMaterialCache& cache = texturedMaterialCache[i];

				for (TexturedMaterialMap::iterator it = cache.map.begin(); it != cache.map.end(); )
					if (it->second.unique())
						it = cache.map.erase(it);
					else
						++it;

				cache.gcSizeLast = cache.map.size();
				cache.gcKeyNext = "";
			}
		}

		MaterialData MaterialGenerator::getParameters(PartMaterial material) {
			MaterialData data;

			switch (material) {
			case ASPHALT_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.635f, 0.0f, 0.025f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case BASALT_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, 1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case BRICK_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.6f, 0.0f, 0.05f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case CARDBOARD_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case CARPET_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.53f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case CERAMIC_TILES_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case CLAY_ROOF_TILES_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.6f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case COBBLESTONE_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, 0.05f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case CONCRETE_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.63f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case CRACKED_LAVA_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 1.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 5.0f, 0.1f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case DIAMONDPLATE_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 1.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, 0.01f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case FABRIC_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.53f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case FOIL_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 1.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, 0.1f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case GLACIER_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.31f, 0.0f, 0.125f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case GRANITE_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 0.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.65f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case GRASS_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 0.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, 0.05f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case GROUND_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, 0.025f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case ICE_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.31f, 0.0f, 0.1f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case LEAFY_GRASS_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, 0.1f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case LEATHER_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.48f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case LIMESTONE_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.53f, 0.0f, 0.1f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case MARBLE_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case METAL_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 1.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case MUD_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, 0.125f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case PAVEMENT_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.6f, 0.0f, 0.05f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case PEBBLE_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.6f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case PLASTER_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case ROCK_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, 0.25f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case ROOF_SHINGLES_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.6f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case RUBBER_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.52f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case RUST_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, -1.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.55f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case SALT_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.53f, 0.0f, 0.1f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case SAND_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.52f, 0.0f, 0.1f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case SANDSTONE_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.65f, 0.0f, 0.25f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case SLATE_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.64f, 0.0f, 0.125f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case SNOW_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.31f, 0.0f, 0.125f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case WOOD_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case WOODPLANKS_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.65f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case PLASTIC_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(0.6f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(0.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.46f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case SMOOTH_PLASTIC_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(0.4f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.46f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case NEON_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(0.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(0.0f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			default: {
				RBXASSERT(false); // Missed a material, or it isn't valid

				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(0.4f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_CCIndexOfRefraction = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			}

			if (data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset.z > 0.0f)
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset.z *= getTiling(material);

			return data;
		}

		uint32_t MaterialGenerator::createFlags(RBX::PartInstance* part, bool skinned) {
			uint32_t materialFlags = 0u;

			if (part->getTransparencyUi() > 0.0f)
				materialFlags |= MaterialGenerator::Flag_Transparent;

			return materialFlags;
		}

		float MaterialGenerator::getTiling(PartMaterial material) {
			switch (material) {
			case ASPHALT_MATERIAL:			return 1.0f;
			case BASALT_MATERIAL:			return 1.0f;
			case BRICK_MATERIAL:			return 0.125f;
			case CARDBOARD_MATERIAL:		return 1.0f;
			case CARPET_MATERIAL:			return 1.0f;
			case CERAMIC_TILES_MATERIAL:	return 1.0f;
			case CLAY_ROOF_TILES_MATERIAL:	return 1.0f;
			case COBBLESTONE_MATERIAL:		return 0.2f;
			case CONCRETE_MATERIAL:			return 0.15f;
			case CRACKED_LAVA_MATERIAL:		return 1.0f;
			case DIAMONDPLATE_MATERIAL:		return 0.2f;
			case FABRIC_MATERIAL:			return 0.15f;
			case FOIL_MATERIAL:				return 0.1f;
			case GLACIER_MATERIAL:			return 1.0f;
			case GRANITE_MATERIAL:			return 0.1f;
			case GRASS_MATERIAL:			return 0.15f;
			case GROUND_MATERIAL:			return 1.0f;
			case ICE_MATERIAL:				return 0.1f;
			case LEAFY_GRASS_MATERIAL:		return 1.0f;
			case LEATHER_MATERIAL:			return 1.0f;
			case LIMESTONE_MATERIAL:		return 1.0f;
			case MARBLE_MATERIAL:			return 0.1f;
			case METAL_MATERIAL:			return 1.0f;
			case MUD_MATERIAL:				return 1.0f;
			case PAVEMENT_MATERIAL:			return 1.0f;
			case PEBBLE_MATERIAL:			return 0.1f;
			case PLASTER_MATERIAL:			return 1.0f;
			case PLASTIC_MATERIAL:			return 1.0f;
			case ROCK_MATERIAL:				return 1.0f;
			case ROOF_SHINGLES_MATERIAL:	return 1.0f;
			case RUBBER_MATERIAL:			return 1.0f;
			case RUST_MATERIAL:				return 0.1f;
			case SALT_MATERIAL:				return 1.0f;
			case SAND_MATERIAL:				return 0.1f;
			case SANDSTONE_MATERIAL:		return 1.0f;
			case SLATE_MATERIAL:			return 0.1f;
			case SNOW_MATERIAL:				return 1.0f;
			case WOOD_MATERIAL:				return 0.2f;
			case WOODPLANKS_MATERIAL:		return 0.2f;

			default:
				RBXASSERT(0); // Missed a new material, or it isn't valid
				return 1.0f;
			}
		}

	}
}
