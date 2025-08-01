#pragma once

#include "GfxBase/GfxPart.h"
#include "util/Extents.h"

#include "RenderQueue.h"

namespace RBX {
	class Contact;
}

namespace RBX {
	namespace Graphics {

		class RenderCamera;
		class RenderQueue;
		class VisualEngine;

		class SpatialHashNode;

		class CullableSceneNode : public RBX::GfxPart {
		public:
			static const bool hasGetFirstContact = false; // simulate __if_exists
			static RBX::Contact* getContact(CullableSceneNode* p0, CullableSceneNode* p1) { RBXASSERT(0); return nullptr; }  // shouldn't be called if hasGetFirstContact is false
			static RBX::Contact* getFirstContact() { RBXASSERT(nullptr); return nullptr; }
			static RBX::Contact* getNextContact(RBX::Contact* prev) { RBXASSERT(nullptr); return nullptr; }
			static uint32_t getNumContacts() { RBXASSERT(nullptr); return 0; }
			static CullableSceneNode* getContactOther(uint32_t id) { RBXASSERT(nullptr); return nullptr; }

			bool requestFixed() const { return false; }
			const Extents& getFastFuzzyExtents() { return worldBounds; }

		public:
			enum CullMode {
				CullMode_None,
				CullMode_Simple,
				CullMode_SpatialHash,
			};

			enum Flags {
				Flags_LightObject = 1 << 0,
				Flags_ShadowCaster = 1 << 1,
			};

			CullableSceneNode(VisualEngine* visualEngine, CullMode cullMode, uint32_t flags);
			~CullableSceneNode();

			void setBlockCount(uint32_t blockCount) { this->blockCount = blockCount; }

			virtual void updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, RenderQueue::Pass pass);

			float getSqDistanceToFocus() const { return sqDistanceToFocus; }

			const Extents& getWorldBounds() const { return worldBounds; }

			void updateWorldBounds(const Extents& aabb);

			VisualEngine* getVisualEngine() const { return visualEngine; }

			uint32_t getFlags() const { return flags; }

			bool updateIsCulledByFRM();

			Vector3 getCenter() const override { return worldBounds.center(); }

		private:
			VisualEngine* visualEngine;

			CullMode cullMode;
			uint32_t flags;

			uint32_t blockCount;
			float sqDistanceToFocus;

			Extents worldBounds;
		};

	}
}
