#include "stdafx.h"
#include "RenderNode.h"

#include "Material.h"

#include "GfxBase/FrameRateManager.h"
#include "VisualEngine.h"
#include "Util.h"
#include "VertexStreamer.h"

namespace RBX {
	namespace Graphics {

		RenderEntity::RenderEntity(RenderNode* node, const GeometryBatch& geometry, const shared_ptr<Material>& material, RenderQueue::Id renderQueueId, unsigned char lodMask)
			: node(node)
			, renderQueueId(renderQueueId)
			, lodMask(lodMask)
			, geometry(geometry)
			, material(material)
		{
		}

		RenderEntity::~RenderEntity()
		{
		}

		void RenderEntity::updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, uint32_t lodIndex, RenderQueue::Pass pass) {
			const shared_ptr<Material>& m = material;

			if (const Technique* technique = m->getBestTechnique(lodIndex, pass)) {
				float distanceKey = (renderQueueId == RenderQueue::Id_Transparent) ? getViewDepth(camera) : 0.0f;

				RenderOperation rop = { this, distanceKey, technique, &geometry };

				queue.getGroup(renderQueueId).push(rop);
			}
		}

		uint32_t RenderEntity::getWorldTransforms4x3(float* buffer, uint32_t maxTransforms, const void** cacheKey) const {
			if (useCache(cacheKey, node)) return 0u;

			RBXASSERT(maxTransforms >= 1u);

			const CoordinateFrame& cframe = node->getCoordinateFrame();

			memcpy(&buffer[0], cframe.rotation[0], sizeof(float) * 3.0f);
			buffer[3] = cframe.translation.x;

			memcpy(&buffer[4], cframe.rotation[1], sizeof(float) * 3.0f);
			buffer[7] = cframe.translation.y;

			memcpy(&buffer[8], cframe.rotation[2], sizeof(float) * 3.0f);
			buffer[11] = cframe.translation.z;

			return 1;
		}

		float RenderEntity::getViewDepth(const RenderCamera& camera) const {
			return computeViewDepth(camera, node->getCoordinateFrame().translation);
		}

		float RenderEntity::computeViewDepth(const RenderCamera& camera, const Vector3& position, float offset) {
			float result = (position - camera.getPosition()).dot(camera.getDirection()) + offset;

			return Math::isNan(result) ? 0.0f : result;
		}

		RenderNode::RenderNode(VisualEngine* visualEngine, CullMode cullMode, uint32_t flags)
			: CullableSceneNode(visualEngine, cullMode, flags)
		{
		}

		RenderNode::~RenderNode() {
			// delete all entities
			for (size_t i = 0u; i < entities.size(); ++i) {
				delete entities[i];
			}
		}

		void RenderNode::addEntity(RenderEntity* entity) {
			RBXASSERT(entity);
			entities.push_back(entity);
		}

		void RenderNode::removeEntity(RenderEntity* entity) {
			std::vector<RenderEntity*>::iterator it = std::find(entities.begin(), entities.end(), entity);
			RBXASSERT(it != entities.end());

			entities.erase(it);
		}

		void RenderNode::updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, RenderQueue::Pass pass) {
			if (updateIsCulledByFRM())
				return;

			FrameRateManager* frm = getVisualEngine()->getFrameRateManager();

			uint32_t lodIndex = frm->getGBufferSetting() ? 0u : getSqDistanceToFocus() < frm->getShadingSqDistance() ? 1u : 2u;

			uint32_t lodMask = 1u << lodIndex;

			// Add all entities
			for (size_t i = 0u; i < entities.size(); ++i)
				if (entities[i]->getLodMask() & lodMask)
					entities[i]->updateRenderQueue(queue, camera, lodIndex, pass);

			// Render bounding box
			if (getVisualEngine()->getSettings()->getDebugShowBoundingBoxes())
				debugRenderBoundingBox();
		}

		void RenderNode::debugRenderBoundingBox() {
			VertexStreamer* vs = getVisualEngine()->getVertexStreamer();
			const Extents& b = getWorldBounds();

			if (!b.isNull()) {
				static const uint32_t edges[12][2] = { {0u, 2u}, {2u, 6u}, {6u, 4u}, {4u, 0u}, {1u, 3u}, {3u, 7u}, {7u, 5u}, {5u, 1u}, {0u, 1u}, {2u, 3u}, {4u, 5u}, {6u, 7u} };

				for (size_t edge = 0u; edge < 12u; ++edge) {
					Vector3 e0 = b.getCorner(edges[edge][0]);
					Vector3 e1 = b.getCorner(edges[edge][1]);

					vs->line3d(e0.x, e0.y, e0.z, e1.x, e1.y, e1.z, Color3::white());
				}
			}
		}

	}
}
