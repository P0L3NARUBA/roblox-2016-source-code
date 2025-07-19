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
//#include "LightGrid.h"
#include "util/SafeToLower.h"

#include "GfxCore/Device.h"
#include "SceneManager.h"
#include "EnvMap.h"

#include "RenderQueue.h"

FASTFLAGVARIABLE(RenderMaterialsOnMobile, true)
FASTFLAGVARIABLE(ForceWangTiles, false)
//FASTFLAG(GlowEnabled)

namespace RBX
{
	namespace Graphics
	{

		// here's how our texture compositing setup works:
		// there is a set of body meshes that covers a canvas area of 1024x512
		static const int kTextureCompositWidth = 1024;
		static const int kTextureCompositHeight = 512;

		// we'd like a 256x512 strip on the side, which contains 1 256x256 slot and 4 128x128 slots
		// this strip should make the base area less wide - therefore we make the canvas wider
		// note that 256 pixels in texture space is more than that in canvas space
		// X pixels in canvas space translate to X / (1024 + X) * 1024 in texture space, so X can be computed using the formula below
		static const float kTextureCompositCanvasExtraSpace = 1024.f * 256.f / (1024.f - 256.f);
		static const float kTextureCompositCanvasWidth = 1024.f + kTextureCompositCanvasExtraSpace;
		static const float kTextureCompositCanvasHeight = 512.f;

		// given a base texture size of 1024, we have to rescale the UVs to fit
		static const float kTextureCompositBaseWidth = 1024.f / kTextureCompositCanvasWidth;
		static const float kTextureCompositExtraWidth = kTextureCompositCanvasExtraSpace / kTextureCompositCanvasWidth;

		// slot configuration in UV space; note that the arrangment is like this:
		// 34
		// 12
		// 00
		// 00
		// with each digit corresponding to a 128x128 area in a 256x512 canvas
		// Vector4 xy is UV offset, zw is UV scale; borders are not included in this table
		static const G3D::Vector4 kTextureCompositSlotConfiguration[] =
		{
			G3D::Vector4(kTextureCompositBaseWidth + 0.0f * kTextureCompositExtraWidth, 0.50f, 1.0f * kTextureCompositExtraWidth, 0.50f),
			G3D::Vector4(kTextureCompositBaseWidth + 0.0f * kTextureCompositExtraWidth, 0.25f, 0.5f * kTextureCompositExtraWidth, 0.25f),
			G3D::Vector4(kTextureCompositBaseWidth + 0.5f * kTextureCompositExtraWidth, 0.25f, 0.5f * kTextureCompositExtraWidth, 0.25f),
			G3D::Vector4(kTextureCompositBaseWidth + 0.0f * kTextureCompositExtraWidth, 0.00f, 0.5f * kTextureCompositExtraWidth, 0.25f),
			G3D::Vector4(kTextureCompositBaseWidth + 0.5f * kTextureCompositExtraWidth, 0.00f, 0.5f * kTextureCompositExtraWidth, 0.25f),
		};

		// one of the slots is used by the head, all other slots are used by accoutrements
		static const size_t kTextureCompositAccoutrementCount = ARRAYSIZE(kTextureCompositSlotConfiguration) - 1;

		// inside each slot we leave a small border of 8x8 pixels to deal with filtering
		static const float kTextureCompositExtraBorderWidth = 8.f / kTextureCompositCanvasWidth;
		static const float kTextureCompositExtraBorderHeight = 8.f / kTextureCompositCanvasHeight;

		// diffuse map is always bound to stage 5
		static const unsigned int kDiffuseMapStage = 5;

		class TextureCompositingDescription
		{
		public:
			TextureCompositingDescription()
			{
				name.reserve(1024);
				layers.reserve(16);

				nameAppend("TexComp");
			}

			void add(const MeshId& mesh, const ContentId& texture, TextureCompositorLayer::CompositMode mode = TextureCompositorLayer::Composit_BlendTexture)
			{
				layers.push_back(TextureCompositorLayer(mesh, texture, mode));

				nameAppend(" T[");
				nameAppend(mesh.toString());
				nameAppend(":");
				nameAppend(texture.toString());
				nameAppend(":");
				nameAppend(mode);
				nameAppend("]");
			}

			void add(const MeshId& mesh, const Color3& color)
			{
				layers.push_back(TextureCompositorLayer(mesh, color));

				nameAppend(" C[");
				nameAppend(mesh.toString());
				nameAppend(":");
				nameAppend(color.toString());
				nameAppend("]");
			}

			const std::string& getName() const
			{
				return name;
			}

			const std::vector<TextureCompositorLayer>& getLayers() const
			{
				return layers;
			}

		private:
			std::string name;
			std::vector<TextureCompositorLayer> layers;

			void nameAppend(const char* value)
			{
				name += value;
			}

			void nameAppend(const std::string& value)
			{
				name += value;
			}

			void nameAppend(int value)
			{
				char buf[32];
				sprintf(buf, "%d", value);

				name += buf;
			}
		};

		struct AccoutrementMesh
		{
			PartInstance* part;
			FileMesh* mesh;
			float quality;
		};

		typedef AccoutrementMesh AccoutrementMeshes[kTextureCompositAccoutrementCount];

		static float getAccoutrementQuality(Accoutrement* acc, PartInstance* part)
		{
			const Vector3& location = acc->getAttachmentPos();
			const Vector3& extents = part->getPartSizeXml();

			// accoutrements are attached to top of the head; Y axis is reversed
			float attachmentTop = -location.y + extents.y / 2;
			float attachmentBottom = -location.y - extents.y / 2;

			// top is below the top of the head - not a hat
			// bottom is significantly below the bottom of the head (head is 1 unit high) - probably not a hat
			if (attachmentTop < 0.f || attachmentBottom < -1.75f) return 0.f;

			// surface area
			return extents.x * extents.y + extents.x * extents.z + extents.y * extents.z;
		}

		struct AccoutrementQualityComparator
		{
			bool operator()(const AccoutrementMesh& lhs, const AccoutrementMesh& rhs) const
			{
				return lhs.quality < rhs.quality;
			}
		};

		struct AccoutrementMeshIdComparator
		{
			bool operator()(const AccoutrementMesh& lhs, const AccoutrementMesh& rhs) const
			{
				return lhs.mesh->getMeshId() < rhs.mesh->getMeshId();
			}
		};

		static void getCompositedAccoutrements(AccoutrementMeshes& result, const HumanoidIdentifier& hi)
		{
			size_t count = 0;

			for (size_t i = 0; i < hi.accoutrements.size(); ++i)
			{
				if (PartInstance* part = hi.accoutrements[i]->findFirstChildOfType<PartInstance>())
				{
					if (FileMesh* mesh = getFileMesh(part))
					{
						if (count < kTextureCompositAccoutrementCount && !mesh->getTextureId().isNull())
						{
							result[count].part = part;
							result[count].mesh = mesh;
							result[count].quality = getAccoutrementQuality(hi.accoutrements[i], part);
							count++;
						}
					}
				}
			}

			if (count > 1)
			{
				// sort all accoutrements by mesh id to keep order stable (this reduces rebaking)
				std::sort(&result[0], &result[count], AccoutrementMeshIdComparator());

				// put the best quality accoutrement to the front to make it use the HQ slot
				AccoutrementMesh* bestQuality = std::max_element(&result[0], &result[count], AccoutrementQualityComparator());

				if (bestQuality->quality > 0)
				{
					std::swap(result[0], *bestQuality);
				}
			}
		}

		static int getExtraSlot(PartInstance* part, const HumanoidIdentifier& hi, const AccoutrementMeshes& accoutrements)
		{
			if (part == hi.head)
				return kTextureCompositAccoutrementCount;

			for (size_t i = 0; i < kTextureCompositAccoutrementCount; ++i)
				if (accoutrements[i].part == part)
					return i;

			return -1;
		}

		static std::pair<bool, G3D::Vector4> getPartCompositConfiguration(PartInstance* part, const HumanoidIdentifier& hi, const AccoutrementMeshes& accoutrements)
		{
			// base part
			if (hi.leftArm == part || hi.leftLeg == part || hi.rightArm == part || hi.rightLeg == part || hi.torso == part)
				return std::make_pair(true, G3D::Vector4(0, 0, kTextureCompositBaseWidth, 1));

			// head and accoutrements occupy the rightmost column, with reversed order (bottom to top)
			int slot = getExtraSlot(part, hi, accoutrements);

			if (slot >= 0)
			{
				const G3D::Vector4& cfg = kTextureCompositSlotConfiguration[slot];
				G3D::Vector4 borderAdjustment(kTextureCompositExtraBorderWidth, kTextureCompositExtraBorderHeight, -2.f * kTextureCompositExtraBorderWidth, -2.f * kTextureCompositExtraBorderHeight);

				return std::make_pair(true, cfg + borderAdjustment);
			}

			return std::make_pair(false, G3D::Vector4());
		}

		static MeshId getExtraSlotMeshId(PartInstance* part, const HumanoidIdentifier& hi, const AccoutrementMeshes& accoutrements)
		{
			int slotId = getExtraSlot(part, hi, accoutrements);
			RBXASSERT(slotId >= 0);

			return MeshId(format("rbxasset://other/CompositExtraSlot%d.mesh", slotId));
		}

		static void prepareHumanoidTextureCompositing(TextureCompositingDescription& desc, const HumanoidIdentifier& hi, const AccoutrementMeshes& accoutrements, CharacterMesh* mesh)
		{
			if (hi.torso)
				desc.add(MeshId("rbxasset://other/CompositTorsoBase.mesh"), hi.torso->getColor());

			if (hi.leftArm)
				desc.add(MeshId("rbxasset://other/CompositLeftArmBase.mesh"), hi.leftArm->getColor());

			if (hi.rightArm)
				desc.add(MeshId("rbxasset://other/CompositRightArmBase.mesh"), hi.rightArm->getColor());

			if (hi.leftLeg)
				desc.add(MeshId("rbxasset://other/CompositLeftLegBase.mesh"), hi.leftLeg->getColor());

			if (hi.rightLeg)
				desc.add(MeshId("rbxasset://other/CompositRightLegBase.mesh"), hi.rightLeg->getColor());

			if (hi.head)
			{
				FileMesh* headMesh = getFileMesh(hi.head);
				MeshId slotMeshId = getExtraSlotMeshId(hi.head, hi, accoutrements);

				desc.add(slotMeshId, hi.head->getColor());

				if (headMesh && !headMesh->getTextureId().isNull())
					desc.add(slotMeshId, headMesh->getTextureId());

				if (hi.head->getChildren())
				{
					const Instances& children = *hi.head->getChildren();

					for (size_t i = children.size(); i > 0; --i)
						if (Decal* decal = Instance::fastDynamicCast<Decal>(children[i - 1].get()))
							if (decal->getFace() == NORM_Z_NEG && !decal->getTexture().isNull())
								desc.add(slotMeshId, decal->getTexture());
				}
			}

			if (mesh && !mesh->getBaseTextureId().isNull())
				desc.add(MeshId("rbxasset://other/CompositFullAtlasBaseTexture.mesh"), mesh->getBaseTextureId());

			if (!hi.pants.isNull())
				desc.add(MeshId("rbxasset://other/CompositPantsTemplate.mesh"), hi.pants);

			if (!hi.shirt.isNull())
				desc.add(MeshId("rbxasset://other/CompositShirtTemplate.mesh"), hi.shirt);

			if (!hi.shirtGraphic.isNull())
				desc.add(MeshId("rbxasset://other/CompositTShirt.mesh"), hi.shirtGraphic);

			if (mesh && !mesh->getOverlayTextureId().isNull())
				desc.add(MeshId("rbxasset://other/CompositFullAtlasOverlayTexture.mesh"), mesh->getOverlayTextureId());

			for (size_t i = 0; i < kTextureCompositAccoutrementCount; ++i)
			{
				if (accoutrements[i].mesh)
				{
					// Accoutrements do not use alpha blending; instead they use alpha test.
					// This means that instead of alpha blend compositing, we have to use a straight blit (so that color stays in tact).
					// In addition to that, texture compositor texture has 1-bit alpha, so the default alpha cutoff is 128;
					// to work with the existing assets, we have to decrease it - we do it using fixed-function 4x modulation,
					// which effectively changes the cutoff to 32.
					MeshId slotMeshId = getExtraSlotMeshId(accoutrements[i].part, hi, accoutrements);

					desc.add(slotMeshId, accoutrements[i].part->getColor());
					desc.add(slotMeshId, accoutrements[i].mesh->getTextureId(), TextureCompositorLayer::Composit_BlitTextureAlphaMagnify4x);
				}
			}
		}

		static std::pair<TextureRef, TextureCompositor::JobHandle> createHumanoidTextureComposit(VisualEngine* visualEngine,
			const HumanoidIdentifier& hi, const AccoutrementMeshes& accoutrements, CharacterMesh* mesh)
		{
			Vector2 canvasSize(kTextureCompositCanvasWidth, kTextureCompositCanvasHeight);

			TextureCompositingDescription desc;
			prepareHumanoidTextureCompositing(desc, hi, accoutrements, mesh);

			TextureCompositor::JobHandle job = visualEngine->getTextureCompositor()->getJob(desc.getName(), hi.humanoid->getFullName() + " Clothes", kTextureCompositWidth, kTextureCompositHeight, canvasSize, desc.getLayers());
			TextureRef texture = visualEngine->getTextureCompositor()->getTexture(job);

			return std::make_pair(texture, job);
		}

		static std::pair<TextureRef, TextureCompositor::JobHandle> createHumanoidTextureCached(VisualEngine* visualEngine,
			const HumanoidIdentifier& hi, const AccoutrementMeshes& accoutrements, CharacterMesh* mesh,
			std::pair<Humanoid*, TextureCompositor::JobHandle>& compositCache)
		{
			if (hi.humanoid == compositCache.first)
			{
				TextureRef texture = visualEngine->getTextureCompositor()->getTexture(compositCache.second);

				return std::make_pair(texture, compositCache.second);
			}

			std::pair<TextureRef, TextureCompositor::JobHandle> result = createHumanoidTextureComposit(visualEngine, hi, accoutrements, mesh);

			compositCache = std::make_pair(hi.humanoid, result.second);

			return result;
		}

		static std::pair<std::pair<TextureRef, TextureCompositor::JobHandle>, G3D::Vector4> createHumanoidTexture(VisualEngine* visualEngine,
			PartInstance* part, const HumanoidIdentifier& hi, unsigned int flags,
			std::pair<Humanoid*, TextureCompositor::JobHandle>& compositCache)
		{
			// we only composit up to a certain number of mesh accoutrements
			AccoutrementMeshes accoutrements = {};

			if (flags & MaterialGenerator::Flag_UseCompositTextureForAccoutrements)
				getCompositedAccoutrements(accoutrements, hi);

			std::pair<bool, G3D::Vector4> cfg = getPartCompositConfiguration(part, hi, accoutrements);

			if (!cfg.first)
				return std::make_pair(std::make_pair(TextureRef(), TextureCompositor::JobHandle()), G3D::Vector4());

			// get mesh that's used as base/overlay texture source
			// heads/accoutrements use torso mesh to share the composit texture
			CharacterMesh* mesh =
				(part == hi.torso || part == hi.leftArm || part == hi.rightArm || part == hi.leftLeg || part == hi.rightLeg)
				? hi.getRelevantMesh(part)
				: hi.torsoMesh;

			// it is possible to assemble a character using several meshes, in which case torso mesh textures are different from i.e. leg mesh textures,
			// making it impossible to use just one composit texture per character. we therefore cache composit texture by humanoid pointer, but only if the mesh has the same configuration as torso
			bool useCache = (mesh == hi.torsoMesh) || (mesh && hi.torsoMesh && mesh->getBaseTextureId() == hi.torsoMesh->getBaseTextureId() && mesh->getOverlayTextureId() == hi.torsoMesh->getOverlayTextureId());

			std::pair<TextureRef, TextureCompositor::JobHandle> texture =
				useCache
				? createHumanoidTextureCached(visualEngine, hi, accoutrements, mesh, compositCache)
				: createHumanoidTextureComposit(visualEngine, hi, accoutrements, mesh);

			return std::make_pair(texture, cfg.second);
		}

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

		int MaterialGenerator::getMaterialId(PartMaterial material) {
			switch (material) {
			case ASPHALT_MATERIAL:			return 1;
			case BASALT_MATERIAL:			return 2;
			case BRICK_MATERIAL:			return 3;
			case CARDBOARD_MATERIAL:		return 4;
			case CARPET_MATERIAL:			return 5;
			case CERAMIC_TILES_MATERIAL:	return 6;
			case CLAY_ROOF_TILES_MATERIAL:	return 7;
			case COBBLESTONE_MATERIAL:		return 8;
			case CONCRETE_MATERIAL:			return 9;
			case CRACKED_LAVA_MATERIAL:		return 10;
			case DIAMONDPLATE_MATERIAL:		return 11;
			case FABRIC_MATERIAL:			return 12;
			case FOIL_MATERIAL:				return 13;
			case GLACIER_MATERIAL:			return 14;
			case GRANITE_MATERIAL:			return 15;
			case GRASS_MATERIAL:			return 16;
			case GROUND_MATERIAL:			return 17;
			case ICE_MATERIAL:				return 18;
			case LEAFY_GRASS_MATERIAL:		return 19;
			case LEATHER_MATERIAL:			return 20;
			case LIMESTONE_MATERIAL:		return 21;
			case MARBLE_MATERIAL:			return 22;
			case METAL_MATERIAL:			return 23;
			case MUD_MATERIAL:				return 24;
			case PAVEMENT_MATERIAL:			return 25;
			case PEBBLE_MATERIAL:			return 26;
			case PLASTER_MATERIAL:			return 27;
			case PLASTIC_MATERIAL:			return 28;
			case ROCK_MATERIAL:				return 29;
			case ROOF_SHINGLES_MATERIAL:	return 30;
			case RUBBER_MATERIAL:			return 31;
			case RUST_MATERIAL:				return 32;
			case SALT_MATERIAL:				return 33;
			case SAND_MATERIAL:				return 34;
			case SANDSTONE_MATERIAL:		return 35;
			case SLATE_MATERIAL:			return 36;
			case SNOW_MATERIAL:				return 37;
			case WOOD_MATERIAL:				return 38;
			case WOODPLANKS_MATERIAL:		return 39;

			default:						return 0;
			}
		}

		static PartMaterial getMaterialFromId(int materialId) {
			switch (materialId) {
			case 1:	 return ASPHALT_MATERIAL;
			case 2:	 return BASALT_MATERIAL;
			case 3:	 return BRICK_MATERIAL;
			case 4:	 return CARDBOARD_MATERIAL;
			case 5:	 return CARPET_MATERIAL;
			case 6:	 return CERAMIC_TILES_MATERIAL;
			case 7:	 return CLAY_ROOF_TILES_MATERIAL;
			case 8:	 return COBBLESTONE_MATERIAL;
			case 9:	 return CONCRETE_MATERIAL;
			case 10: return CRACKED_LAVA_MATERIAL;
			case 11: return DIAMONDPLATE_MATERIAL;
			case 12: return FABRIC_MATERIAL;
			case 13: return FOIL_MATERIAL;
			case 14: return GLACIER_MATERIAL;
			case 15: return GRANITE_MATERIAL;
			case 16: return GRASS_MATERIAL;
			case 17: return GROUND_MATERIAL;
			case 18: return ICE_MATERIAL;
			case 19: return LEAFY_GRASS_MATERIAL;
			case 20: return LEATHER_MATERIAL;
			case 21: return LIMESTONE_MATERIAL;
			case 22: return MARBLE_MATERIAL;
			case 23: return METAL_MATERIAL;
			case 24: return MUD_MATERIAL;
			case 25: return PAVEMENT_MATERIAL;
			case 26: return PEBBLE_MATERIAL;
			case 27: return PLASTER_MATERIAL;
			case 28: return PLASTIC_MATERIAL;
			case 29: return ROCK_MATERIAL;
			case 30: return ROOF_SHINGLES_MATERIAL;
			case 31: return RUBBER_MATERIAL;
			case 32: return RUST_MATERIAL;
			case 33: return SALT_MATERIAL;
			case 34: return SAND_MATERIAL;
			case 35: return SANDSTONE_MATERIAL;
			case 36: return SLATE_MATERIAL;
			case 37: return SNOW_MATERIAL;
			case 38: return WOOD_MATERIAL;
			case 39: return WOODPLANKS_MATERIAL;

			default: return SMOOTH_PLASTIC_MATERIAL;
			}
		}

		/*static Vector4 getLQMatFarTilingFactor(PartMaterial material)
		{
			switch (material)
			{
			case GRANITE_MATERIAL:
			case SLATE_MATERIAL:
			case FOIL_MATERIAL:
			case CONCRETE_MATERIAL:
			case ICE_MATERIAL:
			case GRASS_MATERIAL:
				return Vector4(0.25f, 0.25f, 1, 1);

			case RUST_MATERIAL:
			case COBBLESTONE_MATERIAL:
				return Vector4(0.5f, 0.5f, 1, 1);

			default:
				return Vector4(1, 1, 1, 1);
			}
		}*/

		static PartInstance* getHumanoidFocusPart(const HumanoidIdentifier& hi)
		{
			if (hi.torso) return hi.torso;
			if (hi.head) return hi.head;

			return NULL;
		}

		static bool forceFlatPlastic(DataModelMesh* specialShape)
		{
			if (SpecialShape* shape = specialShape->fastDynamicCast<SpecialShape>())
			{
				return shape->getMeshType() == SpecialShape::HEAD_MESH;
			}
			else
			{
				return false;
			}
		}

#ifdef RBX_PLATFORM_IOS
		static const std::string kTextureExtension = ".pvr";
#elif defined(__ANDROID__)
		static const std::string kTextureExtension = ".pvr";
#else
		static const std::string kTextureExtension = ".dds";
#endif

		static void setupShadowDepthTechnique(Technique& technique)
		{
			// This really culls back faces because SM space has different handedness
			technique.setRasterizerState(RasterizerState::Cull_Front);
		}

		static void setupTechnique(Technique& technique, unsigned int flags, bool hasGlow = false) {
			if (flags & MaterialGenerator::Flag_Transparent) {
				technique.setBlendState(BlendState::Mode_AlphaBlend);

				technique.setDepthState(DepthState(DepthState::Function_GreaterEqual, false));
			}
			else {
				technique.setBlendState(BlendState::Mode_None);

				technique.setDepthState(DepthState(DepthState::Function_GreaterEqual, true));
			}

			/*if (flags & (MaterialGenerator::Flag_ForceDecal | MaterialGenerator::Flag_ForceDecalTexture))
				technique.setRasterizerState(RasterizerState(RasterizerState::Cull_Back, -16));*/

			technique.setRasterizerState(RasterizerState(RasterizerState::Cull_Back, 0));
		}

		static void copyTextureToArray(Texture* targetTexture, Texture* sourceTexture, int materialId = 0u) {
			for (unsigned int i = 0; i < sourceTexture->getMipLevels(); ++i) {
				void* data = nullptr;

				sourceTexture->download(0, i, data, 0u);

				targetTexture->upload(materialId, i, TextureRegion(0u, 0u, 1024u, 1024u), data, sizeof(data));
			}

			targetTexture->commitChanges();
		}

		void MaterialGenerator::setupSmoothPlasticTextures(VisualEngine* visualEngine, Technique& technique) {
			TextureManager* tm = visualEngine->getTextureManager();

			/*technique.setTexture(10, tm->getFallbackTexture(TextureManager::Fallback_White), SamplerState::Filter_Anisotropic);
			technique.setTexture(11, tm->getFallbackTexture(TextureManager::Fallback_BlackTransparent), SamplerState::Filter_Anisotropic);
			technique.setTexture(12, tm->getFallbackTexture(TextureManager::Fallback_BlackTransparent), SamplerState::Filter_Anisotropic);

			technique.setTexture(13, tm->getFallbackTexture(TextureManager::Fallback_NormalMap), SamplerState::Filter_Anisotropic);
			technique.setTexture(14, tm->getFallbackTexture(TextureManager::Fallback_Gray), SamplerState::Filter_Anisotropic);

			technique.setTexture(15, tm->getFallbackTexture(TextureManager::Fallback_BlackTransparent), SamplerState::Filter_Anisotropic);
			technique.setTexture(16, tm->getFallbackTexture(TextureManager::Fallback_BlackTransparent), SamplerState::Filter_Anisotropic);*/
		}

		void MaterialGenerator::setupComplexMaterialTextures(VisualEngine* visualEngine, Technique& technique, const std::string& materialName, int materialId, const std::string& materialVariant) {
			TextureManager* tm = visualEngine->getTextureManager();

			std::string texturePath = "rbxasset://textures/" + materialVariant + "/" + materialName + "/";

			safeToLower(texturePath);
			texturePath.erase(remove(texturePath.begin(), texturePath.end(), ' '), texturePath.end());

			copyTextureToArray(albedoTextures.getTexture().get(), tm->load(ContentId(texturePath + "color" + kTextureExtension), TextureManager::Fallback_White).getTexture().get(), materialId);

			/*Texture* albedoTexture = tm->load(ContentId(texturePath + "color" + kTextureExtension), TextureManager::Fallback_White).getTexture().get();

			for (unsigned int i = 0; albedoTexture->getMipLevels(); ++i) {
				void* data;

				albedoTexture->download(0, i, data);

				albedoTextures.getTexture().get()->upload(materialId, i, TextureRegion(0u, 0u, 1024u, 1024u), data, sizeof(data));
			}*/

			/*technique.setTexture(10, tm->load(ContentId(texturePath + "color" + kTextureExtension), TextureManager::Fallback_White), SamplerState::Filter_Anisotropic);
			technique.setTexture(11, tm->load(ContentId(texturePath + "roughness" + kTextureExtension), TextureManager::Fallback_BlackTransparent), SamplerState::Filter_Anisotropic);
			technique.setTexture(12, tm->load(ContentId(texturePath + "emissive" + kTextureExtension), TextureManager::Fallback_BlackTransparent), SamplerState::Filter_Anisotropic);

			technique.setTexture(13, tm->load(ContentId(texturePath + "normal" + kTextureExtension), TextureManager::Fallback_NormalMap), SamplerState::Filter_Anisotropic);
			technique.setTexture(14, tm->load(ContentId(texturePath + "height" + kTextureExtension), TextureManager::Fallback_Gray), SamplerState::Filter_Anisotropic);

			technique.setTexture(15, tm->load(ContentId(texturePath + "clearcoatA" + kTextureExtension), TextureManager::Fallback_BlackTransparent), SamplerState::Filter_Anisotropic);
			technique.setTexture(16, tm->load(ContentId(texturePath + "clearcoatB" + kTextureExtension), TextureManager::Fallback_BlackTransparent), SamplerState::Filter_Anisotropic);*/

			/*if (wangTileTex)
				technique.setTexture(8, *wangTileTex, SamplerState::Filter_Point);
			else
				technique.setTexture(8, tm->load(ContentId(texturePath + "normaldetail" + kTextureExtension), TextureManager::Fallback_NormalMap), SamplerState::Filter_Anisotropic);*/
		}

		void MaterialGenerator::assignMaterialTextures(VisualEngine* visualEngine, Technique& technique) const {
			technique.setTexture(10, albedoTextures, SamplerState::Filter_Anisotropic);
			technique.setTexture(11, emissiveTextures, SamplerState::Filter_Anisotropic);
			technique.setTexture(12, matValueTextures, SamplerState::Filter_Anisotropic);

			technique.setTexture(13, normalTextures, SamplerState::Filter_Anisotropic);
			technique.setTexture(14, heightTextures, SamplerState::Filter_Anisotropic);

			technique.setTexture(15, clearcoatATextures, SamplerState::Filter_Anisotropic);
			technique.setTexture(16, clearcoatBTextures, SamplerState::Filter_Anisotropic);
		}

		/*static void setupLQMaterialTextures(VisualEngine* visualEngine, Technique& technique, const std::string& materialName)
		{
			TextureManager* tm = visualEngine->getTextureManager();
			std::string texturePath = "rbxasset://textures/" + materialName + "/";

			safeToLower(texturePath);

			technique.setTexture(5, tm->load(ContentId(texturePath + "diffuse" + kTextureExtension), TextureManager::Fallback_White), SamplerState::Filter_Linear);

			if (wangTileTex)
				technique.setTexture(8, *wangTileTex, SamplerState::Filter_Point);
		}*/

		static void setupCommonTextures(VisualEngine* visualEngine, Technique& technique) {
			TextureManager* tm = visualEngine->getTextureManager();
			SceneManager* sceneManager = visualEngine->getSceneManager();

			/* Shadow Maps */
			technique.setTexture(0, sceneManager->getShadowMapAtlas(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));
			technique.setTexture(1, sceneManager->getShadowMapArray(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

			/* Environment BRDF */
			technique.setTexture(2, tm->load(ContentId("rbxasset://textures/brdfLUT" + kTextureExtension), TextureManager::Fallback_None), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

			/* Area Light LTCs */
			technique.setTexture(3, tm->load(ContentId("rbxasset://textures/ltc1LUT" + kTextureExtension), TextureManager::Fallback_None), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));
			technique.setTexture(4, tm->load(ContentId("rbxasset://textures/ltc2LUT" + kTextureExtension), TextureManager::Fallback_None), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

			/* Ambient Occlusion */
			technique.setTexture(5, sceneManager->getAmbientOcclusion(), SamplerState(SamplerState::Filter_Point, SamplerState::Address_Clamp));

			/* Cubemaps */
			technique.setTexture(6, sceneManager->getEnvMap()->getOutdoorTexture(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Wrap));
			technique.setTexture(7, sceneManager->getEnvMap()->getIndoorTextures(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Wrap));
		}

		void MaterialGenerator::setupMaterialTextures(VisualEngine* ve, Technique& technique, PartMaterial renderMaterial) {
			if (renderMaterial == SMOOTH_PLASTIC_MATERIAL)
				setupSmoothPlasticTextures(ve, technique);
			else if (renderMaterial == PLASTIC_MATERIAL)
				setupComplexMaterialTextures(ve, technique, getMaterialName(renderMaterial), getMaterialId(renderMaterial), "global");
			else
				setupComplexMaterialTextures(ve, technique, getMaterialName(renderMaterial), getMaterialId(renderMaterial), "modern");
		}

		void MaterialGenerator::createTextureArrays(VisualEngine* visualEngine) {
			if (albedoTextures.getStatus() == TextureRef::Status_Null) {
				Device* device = visualEngine->getDevice();

				albedoTextures = device->createTexture(Texture::Type_2D_Array, Texture::Format_BC7_sRGB, 1024u, 1024u, 48u, 11u, Texture::Usage_Static);
				emissiveTextures = device->createTexture(Texture::Type_2D_Array, Texture::Format_BC7_sRGB, 1024u, 1024u, 48u, 11u, Texture::Usage_Static);
				matValueTextures = device->createTexture(Texture::Type_2D_Array, Texture::Format_BC7, 1024u, 1024u, 48u, 11u, Texture::Usage_Static);

				normalTextures = device->createTexture(Texture::Type_2D_Array, Texture::Format_BC7, 1024u, 1024u, 48u, 11u, Texture::Usage_Static);
				heightTextures = device->createTexture(Texture::Type_2D_Array, Texture::Format_BC4, 512u, 512u, 48u, 10u, Texture::Usage_Static);

				clearcoatATextures = device->createTexture(Texture::Type_2D_Array, Texture::Format_BC7_sRGB, 1024u, 1024u, 48u, 11u, Texture::Usage_Static);
				clearcoatBTextures = device->createTexture(Texture::Type_2D_Array, Texture::Format_BC7, 1024u, 1024u, 48u, 11u, Texture::Usage_Static);

				for (int i = 0; i == 39; ++i)
					globalMaterialData.Materials[i] = getParameters(getMaterialFromId(i));
			}
		}

		MaterialGenerator::TexturedMaterialCache::TexturedMaterialCache()
			: gcSizeLast(0)
		{
		}

		MaterialGenerator::MaterialGenerator(VisualEngine* visualEngine)
			: visualEngine(visualEngine)
			, compositCache(NULL, TextureCompositor::JobHandle())
		{
			createTextureArrays(visualEngine);
			//wangTilesTex = visualEngine->getTextureManager()->load(ContentId("rbxasset://textures/wangIndex.dds"), TextureManager::Fallback_Black);
		}

		shared_ptr<Material> MaterialGenerator::createBaseMaterial(unsigned int flags)
		{
			unsigned int cacheKey = flags & Flag_CacheMask;

			// Cache lookup
			if (baseMaterialCache[cacheKey])
				return baseMaterialCache[cacheKey];

			// Create material
			shared_ptr<Material> material(new Material());

			std::string vertexSkinning = (flags & Flag_Skinned) ? "Skinned" : "Static";

			if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("BasicMaterialVS", "RegularLOD0FS")) {
				Technique technique(program, 0);

				setupTechnique(technique, flags);

				setupCommonTextures(visualEngine, technique);
				setupSmoothPlasticTextures(visualEngine, technique);
				assignMaterialTextures(visualEngine, technique);

				material->addTechnique(technique);
			}

			/*if ((flags & (Flag_Transparent | Flag_ForceDecal | Flag_ForceDecalTexture)) == 0)
				if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("Default" + vertexSkinning + "HQVS", "DefaultHQGBufferFS"))
				{
					Technique technique(program, 0);

					setupTechnique(technique, flags);

					technique.setTexture(0, TextureRef(), SamplerState::Filter_Linear);

					setupCommonTextures(visualEngine, technique);
					//setupSmoothPlasticTextures(visualEngine, technique);

					material->addTechnique(technique);
				}

			if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("Default" + vertexSkinning + "HQVS", "DefaultHQFS"))
			{
				Technique technique(program, 1);

				setupTechnique(technique, flags);

				technique.setTexture(0, TextureRef(), SamplerState::Filter_Linear);

				setupCommonTextures(visualEngine, technique);
				//setupSmoothPlasticTextures(visualEngine, technique);

				material->addTechnique(technique);
			}

			if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgramOrFFP("Default" + vertexSkinning + "VS", "DefaultFS"))
			{
				Technique technique(program, 2);

				setupTechnique(technique, flags);

				technique.setTexture(0, TextureRef(), SamplerState::Filter_Linear);

				setupCommonTextures(visualEngine, technique);
				//setupSmoothPlasticTextures(visualEngine, technique);

				material->addTechnique(technique);
			}

			if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("DefaultShadow" + vertexSkinning + "VS", "DefaultShadowFS"))
			{
				Technique technique(program, 0, RenderQueue::Pass_Shadows);

				setupShadowDepthTechnique(technique);

				material->addTechnique(technique);
			}*/

			// Fast cache fill
			baseMaterialCache[cacheKey] = material;

			return material;
		}

		shared_ptr<Material> MaterialGenerator::createRenderMaterial(unsigned int flags, PartMaterial renderMaterial) {
			int materialId = getMaterialId(renderMaterial);
			if (materialId < 0) return shared_ptr<Material>();

			RBXASSERT(materialId >= 0 && materialId < ARRAYSIZE(renderMaterialCache));

			unsigned int cacheKey = flags & Flag_CacheMask;

			// Fast cache lookup
			if (renderMaterialCache[materialId][cacheKey])
				return renderMaterialCache[materialId][cacheKey];

			// Create material
			shared_ptr<Material> material(new Material());

			//ContentId studs("rbxasset://textures/studs.dds");

			TextureManager* tm = visualEngine->getTextureManager();

			//std::string materialName = getMaterialName(renderMaterial);

			std::string vertexSkinning = (flags & Flag_Skinned) ? "Skinned" : "Static";
			std::string vertexShader = (renderMaterial == NEON_MATERIAL || renderMaterial == SMOOTH_PLASTIC_MATERIAL) ? "BasicMaterialVS" : "MaterialVS";

			if (renderMaterial == NEON_MATERIAL) {
				if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram(vertexShader, "NeonFS")) {
					Technique technique(program, 0);

					setupTechnique(technique, flags, true);

					material->addTechnique(technique);
				}
			}
			else if (renderMaterial == SMOOTH_PLASTIC_MATERIAL) {
				if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("BasicMaterialVS", "RegularLOD0FS")) {
					Technique technique(program, 0);

					setupTechnique(technique, flags);

					setupCommonTextures(visualEngine, technique);
					setupSmoothPlasticTextures(visualEngine, technique);
					assignMaterialTextures(visualEngine, technique);

					material->addTechnique(technique);
				}
			}
			else {
				if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram(vertexShader, "RegularLOD0FS")) {
					Technique technique(program, 0);

					setupTechnique(technique, flags);

					setupCommonTextures(visualEngine, technique);
					setupMaterialTextures(visualEngine, technique, renderMaterial);
					assignMaterialTextures(visualEngine, technique);

					material->addTechnique(technique);
				}
				else if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("BasicMaterialVS", "FallbackFS")) {
					Technique technique(program, 0);

					setupTechnique(technique, flags);

					material->addTechnique(technique);
				}
			}

			/*if ((flags & Flag_Transparent) == 0)
				if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram(vertexShader, "Default" + materialNameReflectance + "HQGBufferFS"))
				{
					Technique technique(program, 0);

					setupTechnique(technique, flags, hasGlow);

					//technique.setTexture(0, tm->load(studs, TextureManager::Fallback_Gray), SamplerState::Filter_Anisotropic);

					setupCommonTextures(visualEngine, technique);
					setupMaterialTextures(visualEngine, technique, renderMaterial, materialName);

					material->addTechnique(technique);
				}

			if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram(vertexShader, "Default" + materialNameReflectance + "HQFS"))
			{
				Technique technique(program, 1);

				setupTechnique(technique, flags, hasGlow);

				//technique.setTexture(0, tm->load(studs, TextureManager::Fallback_Gray), SamplerState::Filter_Anisotropic);

				setupCommonTextures(visualEngine, technique);
				setupMaterialTextures(visualEngine, technique, renderMaterial, materialName);

				material->addTechnique(technique);
			}

			// LOD shaders for non-plastic materials use low-quality plastic shaders; plastic has reflection even in lod
			std::string lodVS = std::string("Default") + vertexSkinning + (reflectance ? "Reflection" : "") + "VS";
			std::string lodFS = "";

			switch (renderMaterial)
			{
			case RBX::PLASTIC_MATERIAL:
			case RBX::SMOOTH_PLASTIC_MATERIAL:
			{
				lodFS = reflectance ? "Default" + materialNameReflectance + "FS" : "DefaultPlasticFS";
				break;
			}
			case RBX::NEON_MATERIAL:
			{
				lodFS = "DefaultNeonFS";
				break;
			}
			default:
			{
				lodFS = "LowQMaterialFS";

				break;
			}
			}

			if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgramOrFFP(lodVS, lodFS))
			{
				Technique technique(program, 2);

				setupTechnique(technique, flags, hasGlow);

				//technique.setTexture(0, tm->load(ContentId(studs), TextureManager::Fallback_Gray), SamplerState::Filter_Linear);

				setupCommonTextures(visualEngine, technique);

				if (renderMaterial == PLASTIC_MATERIAL || renderMaterial == SMOOTH_PLASTIC_MATERIAL || renderMaterial == NEON_MATERIAL)
					setupSmoothPlasticTextures(visualEngine, technique);
				else
				{
					setupLQMaterialTextures(visualEngine, technique, materialName);
					technique.setConstant("LqmatFarTilingFactor", getLQMatFarTilingFactor(renderMaterial));
				}

				material->addTechnique(technique);
			}

			if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("DefaultShadow" + vertexSkinning + "VS", "DefaultShadowFS"))
			{
				Technique technique(program, 0, RenderQueue::Pass_Shadows);

				setupShadowDepthTechnique(technique);

				material->addTechnique(technique);
			}*/

			// Fast cache fill
			renderMaterialCache[materialId][cacheKey] = material;

			return material;
		}

		shared_ptr<Material> MaterialGenerator::createTexturedMaterial(const TextureRef& texture, const std::string& textureName, unsigned int flags)
		{
			unsigned int cacheKey = flags & Flag_CacheMask;

			// Fast cache lookup
			TexturedMaterialMap::iterator it = texturedMaterialCache[cacheKey].map.find(textureName);

			if (it != texturedMaterialCache[cacheKey].map.end())
				return it->second;

			shared_ptr<Material> baseMaterial = createBaseMaterial(flags);

			if (!baseMaterial)
				return shared_ptr<Material>();

			shared_ptr<Material> material(new Material());

			unsigned int diffuseMapStage = visualEngine->getDevice()->getCaps().supportsFFP ? 0 : kDiffuseMapStage;

			SamplerState::Filter filter = (flags & (Flag_ForceDecal | Flag_ForceDecalTexture)) ? SamplerState::Filter_Anisotropic : SamplerState::Filter_Linear;
			SamplerState::Address address = (flags & Flag_ForceDecal) ? SamplerState::Address_Clamp : SamplerState::Address_Wrap;

			const std::vector<Technique>& techniques = baseMaterial->getTechniques();

			for (size_t i = 0; i < techniques.size(); ++i)
			{
				Technique t = techniques[i];

				t.setTexture(diffuseMapStage, texture, SamplerState(filter, address));

				material->addTechnique(t);
			}

			// Fast cache fill
			texturedMaterialCache[cacheKey].map[textureName] = material;

			return material;
		}

		MaterialGenerator::Result MaterialGenerator::createDefaultMaterial(PartInstance* part, unsigned int flags, PartMaterial renderMaterial)
		{
			PartMaterial actualRenderMaterial = renderMaterial;

#if defined(RBX_PLATFORM_IOS) || defined(__ANDROID__)
			if (!FFlag::RenderMaterialsOnMobile)
			{
				// Force everything to smooth plastic to reduce texture memory
				actualRenderMaterial = SMOOTH_PLASTIC_MATERIAL;
			}
#endif

			unsigned int features = renderMaterial == NEON_MATERIAL ? RenderQueue::Features_Glow : 0;

			return Result(createRenderMaterial(flags, actualRenderMaterial), (flags & Flag_Transparent) ? 0 : Result_PlasticLOD, features);
		}

		MaterialGenerator::Result MaterialGenerator::createMaterialForPart(PartInstance* part, const HumanoidIdentifier* hi, unsigned int flags) {
			if (hi && (flags & Flag_UseCompositTexture)) {
				std::pair<std::pair<TextureRef, TextureCompositor::JobHandle>, G3D::Vector4> htp = createHumanoidTexture(visualEngine, part, *hi, flags, compositCache);

				if (htp.first.first.getTexture()) {
					shared_ptr<Material> material = createTexturedMaterial(htp.first.first, visualEngine->getTextureCompositor()->getTextureId(htp.first.second), flags);

					// attach focus part to texture to make sure texture has an appropriate priority
					visualEngine->getTextureCompositor()->attachInstance(htp.first.second, shared_from(getHumanoidFocusPart(*hi)));

					return Result(material, Result_UsesTexture | Result_UsesCompositTexture, 0, htp.second);
				}
			}

			DataModelMesh* specialShape = getSpecialShape(part);

			if (FileMesh* fileMesh = getFileMesh(specialShape)) {
				const TextureId& textureId = fileMesh->getTextureId();

				TextureRef texture = textureId.isNull() ? TextureRef() : visualEngine->getTextureManager()->load(textureId, TextureManager::Fallback_Gray, fileMesh->getFullName() + ".TextureId");

				return texture.getTexture()
					? Result(createTexturedMaterial(texture, textureId.toString(), flags), Result_UsesTexture)
					: createDefaultMaterial(part, flags, SMOOTH_PLASTIC_MATERIAL);
			}

			if ((flags & Flag_DisableMaterialsAndStuds) != 0 || (specialShape != NULL && forceFlatPlastic(specialShape)))
				return createDefaultMaterial(part, flags, SMOOTH_PLASTIC_MATERIAL);
			else
				return createDefaultMaterial(part, flags, part->getRenderMaterial());
		}

		MaterialGenerator::Result MaterialGenerator::createMaterialForDecal(Decal* decal, unsigned int flags)
		{
			const TextureId& textureId = decal->getTexture();

			TextureRef texture = textureId.isNull() ? TextureRef() : visualEngine->getTextureManager()->load(textureId, TextureManager::Fallback_BlackTransparent, decal->getFullName() + ".Texture");

			return texture.getTexture()
				? Result(createTexturedMaterial(texture, textureId.toString(), flags), Result_UsesTexture)
				: Result();
		}

		MaterialGenerator::Result MaterialGenerator::createMaterial(PartInstance* part, Decal* decal, const HumanoidIdentifier* hi, unsigned int flags)
		{
			if (decal) {
				if (decal->isA<DecalTexture>())
					return createMaterialForDecal(decal, flags | Flag_ForceDecalTexture);
				else
					return createMaterialForDecal(decal, flags | Flag_ForceDecal);
			}
			else
				return createMaterialForPart(part, hi, flags);
		}

		void MaterialGenerator::invalidateCompositCache()
		{
			compositCache = std::make_pair(static_cast<Humanoid*>(NULL), TextureCompositor::JobHandle());
		}

		void MaterialGenerator::garbageCollectIncremental()
		{
			for (unsigned int i = 0; i < ARRAYSIZE(texturedMaterialCache); ++i) {
				TexturedMaterialCache& cache = texturedMaterialCache[i];

				// To catch up with allocation rate we need to visit the number of allocated elements since last run plus a small constant
				size_t visitCount = std::min(cache.map.size(), std::max(cache.map.size(), cache.gcSizeLast) - cache.gcSizeLast + 8);

				TexturedMaterialMap::iterator it = cache.map.find(cache.gcKeyNext);

				for (size_t j = 0; j < visitCount; ++j)
				{
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

		void MaterialGenerator::garbageCollectFull()
		{
			for (unsigned int i = 0; i < ARRAYSIZE(texturedMaterialCache); ++i)
			{
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
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.635f, 0.0f, 0.05f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case BASALT_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, 1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case BRICK_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.6f, 0.0f, 0.05f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case CARDBOARD_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case CARPET_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.53f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case CERAMIC_TILES_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case CLAY_ROOF_TILES_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.6f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case COBBLESTONE_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, 0.05f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case CONCRETE_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.63f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case CRACKED_LAVA_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 1.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 5.0f, 0.1f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case DIAMONDPLATE_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 1.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(2.5f, 0.0f, 0.01f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case FABRIC_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.53f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case FOIL_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 1.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.2f, 0.0f, 0.1f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case GLACIER_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.31f, 0.0f, 0.125f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case GRANITE_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 0.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.65f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case GRASS_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 0.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, 0.05f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case GROUND_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, 0.025f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case ICE_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.31f, 0.0f, 0.1f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case LEAFY_GRASS_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, 0.1f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case LEATHER_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.48f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case LIMESTONE_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.53f, 0.0f, 0.1f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case MARBLE_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case METAL_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 1.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(2.5f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case MUD_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, 0.125f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case PAVEMENT_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.6f, 0.0f, 0.05f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case PEBBLE_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.6f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case PLASTER_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case ROCK_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, 0.25f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case ROOF_SHINGLES_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.6f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case RUBBER_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.52f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case RUST_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, -1.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.55f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case SALT_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.53f, 0.0f, 0.1f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case SAND_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.52f, 0.0f, 0.1f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case SANDSTONE_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.65f, 0.0f, 0.25f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case SLATE_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.64f, 0.0f, 0.125f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case SNOW_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, 1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.31f, 0.0f, 0.125f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case WOOD_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.5f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case WOODPLANKS_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(-1.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(3.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.65f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case PLASTIC_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(0.6f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(0.0f, 1.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.46f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case SMOOTH_PLASTIC_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(0.4f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.46f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			case NEON_MATERIAL: {
				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(0.0f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(0.0f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			default: {
				RBXASSERT(false); // Missed a material, or it isn't valid

				data.RoughnessOverride_MetalnessOverride_AmbientOcclusionFactor_unused = Vector4(0.4f, 0.0f, -1.0f, 0.0f);
				data.AlbedoMode_NormalMapEnabled_ClearcoatEnabled_EmissionMode = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset = Vector4(1.46f, 0.0f, -1.0f, 0.0f);

				data.CCNormalsEnabled_CCFactorOverride_CCRoughnessOverride_unused = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			}

			if (data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset.z > 0.0)
				data.IndexOfRefraction_EmissionFactor_ParallaxFactor_ParallaxOffset.z *= getTiling(material);

			return data;
		}

		unsigned int MaterialGenerator::createFlags(bool skinned, RBX::PartInstance* part, const HumanoidIdentifier* hi, bool& ignoreDecalsOut)
		{
			bool useCompositTexture = hi && hi->humanoid && hi->isPartComposited(part);
			ignoreDecalsOut = false;

			unsigned int materialFlags = skinned ? MaterialGenerator::Flag_Skinned : 0;

			if (hi && part == hi->head && hi->isPartHead(part))
			{
				// Heads don't support materials/studs
				materialFlags |= MaterialGenerator::Flag_DisableMaterialsAndStuds;
			}

			if (useCompositTexture)
			{
				materialFlags |= MaterialGenerator::Flag_UseCompositTexture;

				// Bake all accoutrements in the same composit texture
				materialFlags |= MaterialGenerator::Flag_UseCompositTextureForAccoutrements;

				// If torso has a tshirt, ignore all decals
				if (part == hi->torso && !hi->shirtGraphic.isNull())
					ignoreDecalsOut = true;

				// Ignore head decals since they are composited
				if (part == hi->head)
					ignoreDecalsOut = true;
			}

			if (part->getTransparencyUi() > 0)
			{
				materialFlags |= MaterialGenerator::Flag_Transparent;
			}
			else if (useCompositTexture)
			{
				// Some accoutrements need alpha kill (i.e. feather on a hat)
				// We enable it on all composited materials to batch body parts and accoutrements together
				materialFlags |= MaterialGenerator::Flag_AlphaKill;
			}

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

		unsigned int MaterialGenerator::getDiffuseMapStage()
		{
			return kDiffuseMapStage;
		}

	}
}
