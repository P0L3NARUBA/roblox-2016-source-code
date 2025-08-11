#include "stdafx.h"
#include "FastCluster.h"

#include "humanoid/Humanoid.h"

#include "GfxBase/AsyncResult.h"
#include "GfxBase/PartIdentifier.h"
#include "v8datamodel/Decal.h"
#include "v8datamodel/Accoutrement.h"
#include "v8datamodel/PartCookie.h"

#include "v8world/Clump.h"
#include "Util/Math.h"

#include "SceneManager.h"
#include "GeometryGenerator.h"
#include "MaterialGenerator.h"
#include "Util.h"
#include "SceneUpdater.h"
#include "Material.h"

#include "GfxBase/RenderCaps.h"
#include "GfxBase/FrameRateManager.h"

#include "SuperCluster.h"

#include "VisualEngine.h"

#include "rbx/Profiler.h"

LOGVARIABLE(RenderFastCluster, 0)

namespace RBX {
	namespace Graphics {

		// This should be adjusted so that this number + worst-case number of vertices for any single part <= 65536 (16-bit index buffer limit)
		const uint32_t kFastClusterBatchMaxVertices = 32768u;

		// This should be equal to the maximum number of vertices we can draw in a single call
		const uint32_t kFastClusterBatchGroupMaxVertices = 65535u;

		class FastClusterMeshGenerator {
		public:
			FastClusterMeshGenerator(VisualEngine* visualEngine, RBX::Humanoid* humanoid, uint32_t maxBones, bool isFW)
				: visualEngine(visualEngine)
				, humanoidIdentifier(humanoid)
				, maxBonesPerBatch(0u)
			{
				bones.reserve(maxBones);

				if (isFW || visualEngine->getRenderCaps()->getSkinningBoneCount() == 0u) {
					maxBonesPerBatch = 1u;
				}
				else {
					maxBonesPerBatch = visualEngine->getRenderCaps()->getSkinningBoneCount();
				}

				colorOrderBGR = visualEngine->getDevice()->getCaps().colorOrderBGR;
			}

			~FastClusterMeshGenerator() {
				//visualEngine->getMaterialGenerator()->invalidateCompositCache();
			}

			const HumanoidIdentifier& getHumanoidIdentifier() {
				return humanoidIdentifier;
			}

			void addBone(RBX::PartInstance* root) {
				Bone bone;
				bone.root = root;
				bone.boundsMin = Vector3::maxFinite();
				bone.boundsMax = Vector3::minFinite();

				bones.push_back(bone);
			}

			void addInstance(size_t boneIndex, RBX::PartInstance* part, RBX::Decal* decal, uint32_t materialFlags, RenderQueue::Id renderQueue, RBX::AsyncResult* asyncResult) {
				/*RBXASSERT(boneIndex < bones.size());

				const HumanoidIdentifier* hi = humanoidIdentifier.humanoid ? &humanoidIdentifier : nullptr;

				MaterialGenerator::Result material = visualEngine->getMaterialGenerator()->createMaterial(part, decal, materialFlags);

				if (!material.material)
					return;

				MaterialGroup& mg = materialGroups[std::make_pair(material.material.get(), renderQueue)];

				if (!mg.material) {
					mg.material = material.material;
					mg.renderQueue = renderQueue;
					mg.materialResultFlags = material.flags;
					mg.materialFeatures = material.features;

					if (decal) {
						uint32_t opaqueMaterialFlags = materialFlags & ~MaterialGenerator::Flag_Transparent;

						mg.decalMaterialOpaque = visualEngine->getMaterialGenerator()->createMaterial(part, decal, opaqueMaterialFlags).material;
					}
				}
				else
					RBXASSERT(mg.materialResultFlags == material.flags);

				// force a batch break on transparent objects so that transparency sorting works
				// don't do this if the geometry is for the same part as the last one - this means a decal on different face,
				// decals on different faces of the same part never have transparency ordering issues
				bool forceBatchBreak = renderQueue == RenderQueue::Id_Transparent && part != getLastPart(mg);

				// create new batch on vertex count overflows
				if (mg.batches.empty() || mg.batches.back().counter.getVertexCount() >= kFastClusterBatchMaxVertices || forceBatchBreak)
					mg.batches.push_back(Batch());
				else {
					// create new batch on bone count overflow
					const Batch& batch = mg.batches.back();

					if (batch.bones.size() >= maxBonesPerBatch && batch.bones.back() != boneIndex)
						mg.batches.push_back(Batch());
				}

				Batch& batch = mg.batches.back();

				// fetch any resources the part might need
				uint32_t resourceFlags = ((materialFlags & MaterialGenerator::Flag_UseCompositTexture) || (hi && hi->isPartHead(part)))
					? GeometryGenerator::Resource_SubstituteBodyParts
					: 0u;

				GeometryGenerator::Resources resources = GeometryGenerator::fetchResources(part, hi, resourceFlags, asyncResult);

				// accumulate geometry counters in the batch; actual geometry generation is done in finalize()
				GeometryGenerator::Options options;

				batch.counter.addInstance(part, decal, options, resources);

				// add new bone if needed
				if (batch.bones.empty() || batch.bones.back() != boneIndex) {
					RBXASSERT(batch.bones.size() < maxBonesPerBatch);

					batch.bones.push_back(boneIndex);
				}

				// add batch instance
				BatchInstance bi = { part, decal, batch.bones.size() - 1, material.uvOffsetScale, resources };

				batch.instances.push_back(bi);*/
			}

			unsigned int finalize(FastCluster* cluster, FastClusterSharedGeometry& sharedGeometry) {
				MaterialVertex* sharedVertexData = nullptr;
				uint32_t* sharedIndexData = nullptr;
				uint32_t sharedVertexOffset = 0u;
				uint32_t sharedIndexOffset = 0u;

				// Gather pointers to all batches
				std::vector<std::pair<MaterialGroup*, Batch*> > batches;

				for (MaterialGroupMap::iterator it = materialGroups.begin(); it != materialGroups.end(); ++it) {
					MaterialGroup& mg = it->second;

					for (MaterialGroup::BatchList::iterator bit = mg.batches.begin(); bit != mg.batches.end(); ++bit) {
						Batch& batch = *bit;

						if (batch.instances.empty() || batch.counter.getVertexCount() == 0u || batch.counter.getIndexCount() == 0u)
							continue;

						batches.push_back(std::make_pair(&mg, &batch));
					}
				}

				// Sort batches to make sure batches that can be merged go first
				std::sort(batches.begin(), batches.end(), BatchMaterialPlasticLODComparator());

				// Get total size of the shared geometry data
				uint32_t sharedVertexCount = 0u;
				uint32_t sharedIndexCount = 0u;

				for (size_t i = 0u; i < batches.size(); ++i) {
					Batch& batch = *batches[i].second;

					sharedVertexCount += batch.counter.getVertexCount();
					sharedIndexCount += batch.counter.getIndexCount();
				}

				// Update shared geometry so that it has the required amount of memory
				if (sharedVertexCount > 0u && sharedIndexCount > 0u) {
					setupSharedGeometry(sharedGeometry, sharedVertexCount, sharedIndexCount, cluster->isFW());

					sharedVertexData = static_cast<MaterialVertex*>(sharedGeometry.vertexBuffer->lock(GeometryBuffer::Lock_Discard));
					sharedIndexData = static_cast<uint32_t*>(sharedGeometry.indexBuffer->lock(GeometryBuffer::Lock_Discard));
				}
				else
					sharedGeometry.reset();

				// Create as few geometry objects as we can
				shared_ptr<Geometry> geometry;
				uint32_t geometryBaseVertex = 0u;

				// Create entities in groups
				for (size_t groupBegin = 0u; groupBegin < batches.size(); ) {
					size_t groupEnd = groupBegin + 1u;

					uint32_t groupVertexCount = batches[groupBegin].second->counter.getVertexCount();

					uint32_t groupVertexOffset = sharedVertexOffset;
					uint32_t groupIndexOffset = sharedIndexOffset;
					Extents groupBounds;

					// Reuse geometry if generated indices stay within 16-bit boundaries
					if (!geometry || groupVertexOffset + groupVertexCount - geometryBaseVertex > kFastClusterBatchGroupMaxVertices) {
						geometry = visualEngine->getDevice()->createGeometry(getVertexLayout(), sharedGeometry.vertexBuffer, sharedGeometry.indexBuffer, groupVertexOffset);
						geometryBaseVertex = groupVertexOffset;
					}

					// Generate geometry for high LOD
					for (size_t bi = groupBegin; bi < groupEnd; ++bi) {
						const MaterialGroup& mg = *batches[bi].first;
						const Batch& batch = *batches[bi].second;

						uint32_t vertexOffset = sharedVertexOffset - geometryBaseVertex;

						// generate geometry
						std::vector<uint32_t> instanceVertexCount;

						Extents bounds = generateBatchGeometry(mg, batch, sharedVertexData + geometryBaseVertex, sharedIndexData + sharedIndexOffset, vertexOffset, instanceVertexCount, cluster->isFW());

						// create geometry batch
						GeometryBatch geometryBatch(geometry, Geometry::Primitive_Triangles, sharedIndexOffset, batch.counter.getIndexCount(), vertexOffset);

						// setup entity
						uint8_t lodMask = (groupEnd - groupBegin > 1u) ? ((1u << 0u) | (1u << 1u)) : 0xffu;

						// override render queue if character will cast shadow
						RenderQueue::Id queueId = mg.renderQueue;

						if (mg.renderQueue == RenderQueue::Id_Opaque && cluster->getHumanoidKey())
							queueId = RenderQueue::Id_OpaqueCasters;
						if (mg.renderQueue == RenderQueue::Id_Transparent && cluster->getHumanoidKey())
							queueId = RenderQueue::Id_TransparentCasters;

						//cluster->addEntity(new FastClusterEntity(cluster, geometryBatch, mg.material, mg.decalMaterialOpaque, queueId, lodMask, batch.bones, bounds, mg.materialFeatures));

						// update buffer offsets
						sharedVertexOffset += batch.counter.getVertexCount();
						sharedIndexOffset += batch.counter.getIndexCount();

						groupBounds.expandToContain(bounds);
					}

					// Create merged entity for low LOD
					if (groupEnd - groupBegin > 1u) {
						const MaterialGroup& mg = *batches[groupBegin].first;
						const Batch& batch = *batches[groupBegin].second;

						// create geometry batch
						GeometryBatch geometryBatch(geometry, Geometry::Primitive_Triangles, groupIndexOffset, sharedIndexOffset - groupIndexOffset, groupVertexOffset - geometryBaseVertex);

						// setup entity
						//cluster->addEntity(new FastClusterEntity(cluster, geometryBatch, mg.material, mg.decalMaterialOpaque, mg.renderQueue, /* lodMask= */ uint8_t(1u << 2u), batch.bones, groupBounds, mg.materialFeatures));
					}

					// Move to next batch portion
					groupBegin = groupEnd;
				}

				if (sharedVertexData && sharedIndexData) {
					RBXASSERT(sharedVertexOffset == sharedVertexCount && sharedIndexOffset == sharedIndexCount);

					RBXPROFILER_SCOPE("Render", "upload");

					sharedGeometry.vertexBuffer->unlock();
					sharedGeometry.indexBuffer->unlock();
				}

				return sharedVertexCount;
			}

			uint32_t getBoneCount() {
				return bones.size();
			}

			RBX::PartInstance* getBoneRoot(uint32_t i) {
				return bones[i].root;
			}

			Extents getBoneBounds(uint32_t i) {
				const Bone& bone = bones[i];

				return Extents(bone.boundsMin, bone.boundsMax);
			}

		private:
			struct BatchInstance {
				RBX::PartInstance* part;
				RBX::Decal* decal;
				uint32_t localBoneIndex;
				G3D::Vector4 uvOffsetScale;
				GeometryGenerator::Resources resources;
			};

			struct Batch {
				GeometryGenerator counter;
				std::vector<BatchInstance> instances;
				std::vector<uint32_t> bones;
			};

			struct MaterialGroup
			{
				shared_ptr<Material> material;
				RenderQueue::Id renderQueue;

				shared_ptr<Material> decalMaterialOpaque;
				uint32_t materialResultFlags;
				uint32_t materialFeatures;

				typedef std::list<Batch> BatchList;
				BatchList batches;
			};

			struct Bone {
				RBX::PartInstance* root;
				RBX::Vector3 boundsMin;
				RBX::Vector3 boundsMax;
			};

			typedef std::map<std::pair<Material*, RenderQueue::Id>, MaterialGroup> MaterialGroupMap;

			VisualEngine* visualEngine;
			HumanoidIdentifier humanoidIdentifier;

			uint32_t maxBonesPerBatch;
			bool colorOrderBGR;

			MaterialGroupMap materialGroups;
			std::vector<Bone> bones;

			RBX::PartInstance* getLastPart(const MaterialGroup& mg) {
				if (mg.batches.empty())
					return nullptr;

				const Batch& batch = mg.batches.back();

				if (batch.instances.empty())
					return nullptr;

				return batch.instances.back().part;
			}

			const shared_ptr<VertexLayout>& getVertexLayout() {
				shared_ptr<VertexLayout>& p = visualEngine->getFastClusterLayout();

				if (!p) {
					std::vector<VertexLayout::Element> elements;

					/*elements.push_back(VertexLayout::Element(0u, 0u, VertexLayout::Format_Float3, VertexLayout::Input_Vertex, VertexLayout::Semantic_Position));
					elements.push_back(VertexLayout::Element(0u, 12u, VertexLayout::Format_Float2, VertexLayout::Input_Vertex, VertexLayout::Semantic_Texture));
					elements.push_back(VertexLayout::Element(0u, 20u, VertexLayout::Format_Float4, VertexLayout::Input_Vertex, VertexLayout::Semantic_Color));
					elements.push_back(VertexLayout::Element(0u, 36u, VertexLayout::Format_Float3, VertexLayout::Input_Vertex, VertexLayout::Semantic_Normal));
					elements.push_back(VertexLayout::Element(0u, 48u, VertexLayout::Format_Float3, VertexLayout::Input_Vertex, VertexLayout::Semantic_Tangent));*/

					//bool useShaders = visualEngine->getRenderCaps()->getSkinningBoneCount() > 0;

					p = visualEngine->getDevice()->createVertexLayout(elements);
					RBXASSERT(p);
				}

				return p;
			}

			bool canUseBuffer(uint32_t currentCount, uint32_t desiredCount) {
				// We can use the current buffer if it has enough data and it does not waste >10% of memory
				return
					desiredCount <= currentCount &&
					currentCount - desiredCount < currentCount / 10u;
			}

			void setupSharedGeometry(FastClusterSharedGeometry& sharedGeometry, uint32_t vertexCount, uint32_t indexCount, bool isFW) {
				// Create vertex buffer
				if (!sharedGeometry.vertexBuffer || !canUseBuffer(sharedGeometry.vertexBuffer->getElementCount(), vertexCount))
					sharedGeometry.vertexBuffer = visualEngine->getDevice()->createVertexBuffer(sizeof(MaterialVertex), vertexCount, GeometryBuffer::Usage_Static);

				// Create index buffer
				if (!sharedGeometry.indexBuffer || !canUseBuffer(sharedGeometry.indexBuffer->getElementCount(), indexCount))
					sharedGeometry.indexBuffer = visualEngine->getDevice()->createIndexBuffer(sizeof(uint16_t), indexCount, GeometryBuffer::Usage_Static);
			}

			CoordinateFrame getRelativeTransform(PartInstance* part, PartInstance* root) {
				if (part == root) {
					static const CoordinateFrame identity;

					return identity;
				}
				else if (root) {
					const CoordinateFrame& partFrame = part->getCoordinateFrame();
					const CoordinateFrame& rootFrame = root->getCoordinateFrame();

					return rootFrame.toObjectSpace(partFrame);
				}
				else
					return part->getCoordinateFrame();
			}

			Extents generateBatchGeometry(const MaterialGroup& mg, const Batch& batch, MaterialVertex* vbptr, uint32_t* ibptr, uint32_t vertexOffset, std::vector<uint32_t>& instanceVertexCount, bool isFW) {
				RBXPROFILER_SCOPE("Render", "generateBatchGeometry");

				instanceVertexCount.reserve(batch.instances.size());

				GeometryGenerator generator(vbptr, ibptr, vertexOffset);

				Vector3 boundsMin = Vector3::maxFinite();
				Vector3 boundsMax = Vector3::minFinite();

				const HumanoidIdentifier* hi = humanoidIdentifier.humanoid ? &humanoidIdentifier : nullptr;

				for (size_t i = 0u; i < batch.instances.size(); ++i) {
					const BatchInstance& bi = batch.instances[i];
					Bone& bone = bones[batch.bones[bi.localBoneIndex]];

					GeometryGenerator::Options options(visualEngine, *bi.part, bi.decal, getRelativeTransform(bi.part, bone.root), mg.materialResultFlags, bi.uvOffsetScale);

					generator.resetBounds();

					size_t oldVertexCount = generator.getVertexCount();

					generator.addInstance(bi.part, bi.decal, options, bi.resources, hi);

					instanceVertexCount.push_back(generator.getVertexCount() - oldVertexCount);

					if (generator.areBoundsValid()) {
						RBX::AABox partBounds = generator.getBounds();

						bone.boundsMin = bone.boundsMin.min(partBounds.low());
						bone.boundsMax = bone.boundsMax.max(partBounds.high());
						boundsMin = boundsMin.min(partBounds.low());
						boundsMax = boundsMax.max(partBounds.high());
					}
				}

				RBXASSERT(generator.getVertexCount() == batch.counter.getVertexCount() + vertexOffset && generator.getIndexCount() == batch.counter.getIndexCount());

				return getBounds(boundsMin, boundsMax);
			}

			Extents getBounds(const Vector3& min, const Vector3& max) {
				if (min.x <= max.x)
					return Extents(min, max);
				else
					return Extents();
			}

			struct BatchMaterialPlasticLODComparator {
				bool operator()(const std::pair<MaterialGroup*, Batch*>& lhs, const std::pair<MaterialGroup*, Batch*>& rhs) const {
					MaterialGroup* ml = lhs.first;
					MaterialGroup* mr = rhs.first;

					return (ml->materialResultFlags & MaterialGenerator::Result_UsesTexture) > (mr->materialResultFlags & MaterialGenerator::Result_UsesTexture);
				}
			};
		};

		struct PartBindingNullPredicate {
			template <typename T> bool operator()(const T& part) const {
				return part.binding == nullptr;
			}
		};

		struct PartClumpSinglePredicate {
			template <typename T> bool operator()(const T& part) const {
				const Primitive* p = part.instance->getConstPartPrimitive();
				const Primitive* clumpRoot = p->getRoot<Primitive>();

				return clumpRoot->numChildren() == 0u;
			}
		};

		struct PartClumpGroupPredicate {
			template <typename T> bool operator()(const T& lhs, const T& rhs) const {
				return lhs.instance->getClump() < rhs.instance->getClump();
			}
		};

		FastClusterEntity::FastClusterEntity(FastCluster* cluster, const GeometryBatch& geometry, const shared_ptr<Material>& material, const shared_ptr<Material>& decalMaterialOpaque,
			RenderQueue::Id renderQueueId, uint8_t lodMask, const std::vector<uint32_t>& bones, const Extents& localBounds, uint32_t extraFeatures)
			: RenderEntity(cluster, geometry, material, renderQueueId, lodMask)
			, decalMaterialOpaque(decalMaterialOpaque)
			, extraFeatures(extraFeatures)
			, bones(bones)
			, localBoundsCenter(localBounds.center())
			, sortKeyOffset(0.0f)
		{
			/*if (decalMaterialOpaque) {
				if (const Technique* technique = decalMaterialOpaque->getBestTechnique(0u, RenderQueue::Pass_Default))
					decalTexture = technique->getTexture(MaterialGenerator::getDiffuseMapStage());

				// if decal we want to offset slightly sort key
				sortKeyOffset = -0.01f;
			}*/
		}

		FastClusterEntity::~FastClusterEntity()
		{
		}

		/*uint32_t FastClusterEntity::getWorldTransforms4x3(float* buffer, uint32_t maxTransforms, const void** cacheKey) const {
			if (useCache(cacheKey, this)) return 0u;

			RBXASSERT(bones.size() <= maxTransforms);

			FastCluster* cluster = static_cast<FastCluster*>(node);

			for (uint32_t i = 0u; i < bones.size(); ++i) {
				const CoordinateFrame& cframe = cluster->getTransform(bones[i]);

				memcpy(&buffer[0], cframe.rotation[0], sizeof(float) * 3u);
				buffer[3] = cframe.translation.x;

				memcpy(&buffer[4], cframe.rotation[1], sizeof(float) * 3u);
				buffer[7] = cframe.translation.y;

				memcpy(&buffer[8], cframe.rotation[2], sizeof(float) * 3u);
				buffer[11] = cframe.translation.z;

				buffer += 12;
			}

			return bones.size();
		}*/

		void FastClusterEntity::updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, uint32_t lodIndex, RenderQueue::Pass pass) {
			TextureRef::Status decalStatus = decalTexture.getStatus();

			if (decalStatus != TextureRef::Status_Null) {
				if (decalStatus != TextureRef::Status_Loaded)
					return;

				if (renderQueueId == RenderQueue::Id_Decals) {
					if (!decalTexture.getInfo().alpha) {
						material = decalMaterialOpaque;
						renderQueueId = RenderQueue::Id_OpaqueDecals;
					}
					else
						renderQueueId = RenderQueue::Id_TransparentDecals;
				}

				decalTexture = TextureRef();
			}

			queue.setFeature(extraFeatures);

			RenderEntity::updateRenderQueue(queue, camera, lodIndex, pass);
		}

		float FastClusterEntity::getViewDepth(const RenderCamera& camera) const {
			// getViewDepth should only be used for transparency sorting; then there should only be one bone
			RBXASSERT(bones.size() == 1u);

			const CoordinateFrame& transform = static_cast<FastCluster*>(node)->getTransform(bones[0]);

			Vector3 center = transform.pointToWorldSpace(localBoundsCenter);

			return RenderEntity::computeViewDepth(camera, center, sortKeyOffset);
		}

		FastClusterBinding::FastClusterBinding(FastCluster* cluster, const shared_ptr<PartInstance>& part)
			: GfxBinding(part)
			, cluster(cluster)
		{
			bindProperties(part);

			RBXASSERT(part->getGfxPart() == nullptr);
			part->setGfxPart(cluster);

			FASTLOG4(FLog::RenderFastCluster, "FastCluster[%p]: bound part %p to binding %p (%d connections)", cluster, part.get(), this, connections.size());

		}

		void FastClusterBinding::invalidateEntity() {
			FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: part %p with binding %p requests invalidateEntity", cluster, partInstance.get(), this);

			cluster->invalidateEntity();
		}

		void FastClusterBinding::onCoordinateFrameChanged() {
			if (cluster->isFW()) {
				FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: part %p with binding %p requests coordinate frame change", cluster, partInstance.get(), this);

				cluster->invalidateEntity();
				cluster->queueClusterCheck();
				//cluster->markLightingDirty();
			}
		}

		void FastClusterBinding::onSizeChanged() {
			FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: part %p with binding %p requests size change", cluster, partInstance.get(), this);

			cluster->invalidateEntity();
			cluster->queueClusterCheck();
			//cluster->markLightingDirty();
		}

		void FastClusterBinding::onTransparencyChanged() {
			FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: part %p with binding %p requests transparency change", cluster, partInstance.get(), this);

			cluster->invalidateEntity();
			//cluster->markLightingDirty();
		}

		void FastClusterBinding::onSpecialShapeChanged() {
			FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: part %p with binding %p requests special shape change", cluster, partInstance.get(), this);

			cluster->invalidateEntity();
			//cluster->markLightingDirty();
		}

		void FastClusterBinding::unbind() {
			FASTLOG4(FLog::RenderFastCluster, "FastCluster[%p]: unbind part %p from binding %p (%d connections)", cluster, partInstance.get(), this, connections.size());

			RBXASSERT(partInstance->getGfxPart() == cluster || partInstance->getGfxPart() == nullptr);
			GfxBinding::unbind();
		}

		FastClusterSharedGeometry::FastClusterSharedGeometry()
		{
		}

		void FastClusterSharedGeometry::reset() {
			vertexBuffer.reset();
			indexBuffer.reset();
		}

		FastCluster::FastCluster(VisualEngine* visualEngine, Humanoid* humanoid, SuperCluster* owner, bool fw)
			: RenderNode(visualEngine, CullMode_SpatialHash, humanoid ? Flags_ShadowCaster : 0u)
			, humanoid(humanoid)
			, owner(owner)
			, fw(fw)
			, dirty(false)
		{
			if (humanoid)
				FASTLOG2(FLog::RenderFastCluster, "FastCluster[%p]: create (humanoid %p)", this, humanoid);
			else {
				const SpatialGridIndex& spatialIndex = owner->getSpatialIndex();
				FASTLOG5(FLog::RenderFastCluster, "FastCluster[%p]: create (grid %dx%dx%d, flags %d)", this, spatialIndex.position.x, spatialIndex.position.y, spatialIndex.position.z, spatialIndex.flags);
			}

			SceneUpdater* su = visualEngine->getSceneUpdater();

			// register non-FW clusters for updateCoordinateFrame
			if (!fw)
				su->notifyAwake(this);

			getStatsBucket().clusters++;
		}


		FastCluster::~FastCluster() {
			unbind();

			// delete shared geometry
			sharedGeometry.reset();

			getStatsBucket().clusters--;

			// notify scene updater about destruction so that the pointer to FastCluster is no longer stored
			getVisualEngine()->getSceneUpdater()->notifyDestroyed(this);
		}

		void FastCluster::addPart(const boost::shared_ptr<PartInstance>& part) {
			FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: addPart %p (part count was %d)", this, part.get(), parts.size());
			FASTLOGS(FLog::RenderFastCluster, "FastCluster part: %s", part->getFullName().c_str());

			Part p;
			p.instance = part.get();
			p.binding = new FastClusterBinding(this, part);

			parts.push_back(p);

			getStatsBucket().parts++;

			lightDirty = true;
		}

		void FastCluster::checkCluster() {
			if (!owner) return;

			const SpatialGridIndex& spIndex = getSpatialIndex();

			FASTLOG5(FLog::RenderFastCluster, "FastCluster[%p]: checking cluster (spatial index %dx%dx%d-%u)", this, spIndex.position.x, spIndex.position.y, spIndex.position.z, spIndex.flags);

			SceneUpdater* sceneUpdater = getVisualEngine()->getSceneUpdater();

			bool selfInvalidate = false;

			for (size_t i = 0u; i < parts.size(); ++i) {
				Part& part = parts[i];

				if (!part.binding->isBound())
					continue;
				else {
					bool isPartFW = SceneUpdater::isPartStatic(part.instance);
					SpatialGridIndex newspIndex;

					newspIndex = owner->getSpatialGrid()->getIndexUnsafe(part.instance, isPartFW ? SpatialGridIndex::fFW : 0u);

					static Vector3int16 m(-1, -1, -1), M(1, 1, 1);

					if (spIndex.flags != newspIndex.flags || !(spIndex.position - newspIndex.position).isBetweenInclusive(m, M)) {
						if (selfInvalidate == false) {
							bool result = sceneUpdater->checkAddSeenFastClusters(newspIndex);
							RBXASSERT(result);
							priorityInvalidateEntity();
						}

						if (!sceneUpdater->checkAddSeenFastClusters(spIndex)) {
							RBXASSERT(selfInvalidate == true); // Should never fail on first added cluster
							queueClusterCheck();
							break;
						}

						if (spIndex.flags != newspIndex.flags)
							FASTLOG4(FLog::RenderFastCluster, "FastCluster[%p]: part %p changed FW status from %d to %d", this, part.instance, spIndex.flags, newspIndex.flags);
						else
							FASTLOG5(FLog::RenderFastCluster, "FastCluster[%p]: part %p changed spatial index to %dx%dx%d", this, part.instance, spIndex.position.x, spIndex.position.y, spIndex.position.z);

						boost::shared_ptr<PartInstance> instance = shared_from(part.instance);

						part.binding->unbind();
						delete part.binding;
						part.binding = nullptr;

						getVisualEngine()->getSceneUpdater()->queuePartToCreate(instance);

						getStatsBucket().parts--;

						selfInvalidate = true;

						lightDirty = true;
					}
				}
			}

			if (selfInvalidate) {
				parts.erase(std::remove_if(parts.begin(), parts.end(), PartBindingNullPredicate()), parts.end());
				owner->checkCluster(this);
			}
		}

		void FastCluster::invalidateEntity() {
			FASTLOG1(FLog::RenderFastCluster, "FastCluster[%p]: invalidateEntity", this);

			if (!dirty) {
				dirty = true;

				getVisualEngine()->getSceneUpdater()->queueInvalidateFastCluster(this);
			}
		}

		void FastCluster::priorityInvalidateEntity() {
			FASTLOG1(FLog::RenderFastCluster, "FastCluster[%p]: priorityInvalidateEntity", this);

			dirty = true;

			getVisualEngine()->getSceneUpdater()->queuePriorityInvalidateFastCluster(this);
		}


		void FastCluster::checkBindings() {
			for (size_t i = 0u; i < parts.size(); ++i) {
				Part& part = parts[i];

				if (!part.binding->isBound()) {
					FASTLOG2(FLog::RenderFastCluster, "FastCluster[%p]: part %p is no longer in workspace", this, part.instance);

					delete part.binding;
					part.binding = nullptr;
				}
				else {
					Humanoid* newHumanoid = SceneUpdater::getHumanoid(part.instance);

					if (humanoid != newHumanoid) {
						FASTLOG4(FLog::RenderFastCluster, "FastCluster[%p]: part %p changed humanoid from %p to %p", this, part.instance, newHumanoid, humanoid);

						boost::shared_ptr<PartInstance> instance = shared_from(part.instance);

						part.binding->unbind();
						delete part.binding;
						part.binding = nullptr;

						getVisualEngine()->getSceneUpdater()->queuePartToCreate(instance);
					}
				}

				if (!part.binding) {
					getStatsBucket().parts--;

					lightDirty = true;
				}
			}

			parts.erase(std::remove_if(parts.begin(), parts.end(), PartBindingNullPredicate()), parts.end());
		}

		void FastCluster::updateEntity(bool assetsUpdated) {
			if (!assetsUpdated && !dirty) {
				FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: skipping updateEntity dirty: %d, assetsUpdated: %d", this, dirty, assetsUpdated);
				return;
			}

			RBX::Timer<RBX::Time::Precise> timer;

			// cluster is dirty at this point if updateEntity is a reaction to invalidateEntity, but it may not be dirty if
			// scene updater decides to update the cluster after a requested asset is ready.
			FASTLOG4(FLog::RenderFastCluster, "FastCluster[%p]: updateEntity for %d parts (dirty: %d, light dirty: %d)", this, parts.size(), dirty, lightDirty);

			// reset dirty state before updating to check it after updating finished
			dirty = false;

			// update all part bindings
			checkBindings();

			// invalidate lighting for the old AABB
			/*if (lightDirty)
				invalidateLighting(getWorldBounds());*/

				// if we don't need this cluster any more, destroy it
			if (parts.empty()) {
				FASTLOG1(FLog::RenderFastCluster, "FastCluster[%p]: destroy (no more parts)", this);

				if (owner)
					owner->destroyFastCluster(this); // this call deletes the object, should be the last call in this function
				else
					getVisualEngine()->getSceneUpdater()->destroyFastCluster(this); // this call deletes the object, should be the last call in this function
				return;
			}

			// update cluster geometry
			AsyncResult asyncResult;

			updateClumpGrouping();
			uint32_t totalVertexCount = updateGeometry(&asyncResult);

			// subscribe for updateEntity when some pending assets are done loading
			getVisualEngine()->getSceneUpdater()->notifyWaitingForAssets(this, asyncResult.waitingFor);

			// update block count used for FRM-based culling
			setBlockCount(parts.size());

			// this call updates the AABB
			updateCoordinateFrame(false);

			// invalidate lighting for the new AABB
			/*if (lightDirty)
				invalidateLighting(getWorldBounds());*/

				// reset light dirty; this means that any pending changes except for the changes in bone transforms have resulted in light invalidation
			lightDirty = false;

			// we should not do invalidateEntity from updateEntity - this results in excessive invalidations
			RBXASSERT(!dirty);

			FASTLOG5(FLog::RenderFastCluster, "FastCluster[%p]: updated geometry for %d parts in %d usec (%d entities, %d vertices)", this, parts.size(), (int)(timer.delta().msec() * 1000.0), entities.size(), totalVertexCount);
		}

		void FastCluster::updateClumpGrouping() {
			// no need to update grouping for FW
			if (fw) return;

			// move all single-clump parts to the beginning (to make sorting cheaper)
			std::vector<Part>::iterator middle = std::partition(parts.begin(), parts.end(), PartClumpSinglePredicate());

			// sort remaining parts by clump to use one bone for clump
			std::sort(middle, parts.end(), PartClumpGroupPredicate());

			FASTLOG4(FLog::RenderFastCluster, "FastCluster[%p]: %d parts, %d single-clump parts, %d multi-clump parts", this, parts.size(), middle - parts.begin(), parts.end() - middle);
		}

		uint32_t FastCluster::updateGeometry(AsyncResult* asyncResult) {
			RBXPROFILER_SCOPE("Render", "updateGeometry");

			FastClusterMeshGenerator generator(getVisualEngine(), humanoid, parts.size(), fw);

			const HumanoidIdentifier& hi = generator.getHumanoidIdentifier();

			// for FW cluster all parts use one pseudo-bone
			if (fw)
				generator.addBone(nullptr);

			// generate geometry for parts
			RBX::Clump* lastClump = nullptr;

			for (size_t parti = 0u; parti < parts.size(); ++parti) {
				Part& part = parts[parti];

				// Material flags
				bool ignoreDecals = false;
				uint32_t materialFlags = MaterialGenerator::createFlags(part.instance, !fw);
				bool partTransparent = (part.instance->getTransparencyUi() > 0.0f);

				// add a new bone if necessary
				Clump* clump = part.instance->getClump();

				if (!fw && (!clump || clump != lastClump)) {
					generator.addBone(part.instance);

					lastClump = clump;
				}

				uint32_t boneIndex = generator.getBoneCount() - 1u;

				// add part geometry
				if (part.instance->getTransparencyUi() < 1.0f)
					generator.addInstance(boneIndex, part.instance, nullptr, materialFlags, partTransparent ? RenderQueue::Id_Transparent : RenderQueue::Id_Opaque, asyncResult);

				// add part decals / textures
				if ((part.instance->getCookie() & PartCookie::HAS_DECALS) && part.instance->getChildren() && !ignoreDecals) {
					uint32_t decalMaterialFlags = (materialFlags & MaterialGenerator::Flag_Skinned) | MaterialGenerator::Flag_Transparent;

					const Instances& children = *part.instance->getChildren();

					for (size_t i = 0u; i < children.size(); ++i) {
						if (Decal* decal = children[i]->fastDynamicCast<Decal>()) {
							float decalTransparency = decal->getTransparencyUi();

							if (decalTransparency < 1.0f) {
								RenderQueue::Id renderQueue = partTransparent ? RenderQueue::Id_Transparent : decalTransparency > 0.0f ? RenderQueue::Id_TransparentDecals : RenderQueue::Id_Decals;

								generator.addInstance(boneIndex, part.instance, decal, decalMaterialFlags, renderQueue, asyncResult);
							}
						}
					}
				}
			}

			// Destroy all existing entities
			for (size_t i = 0u; i < entities.size(); ++i)
				delete entities[i];

			entities.clear();

			// Generate new entities
			uint32_t vertexCount = generator.finalize(this, sharedGeometry);

			// Retrieve bone data
			bones.resize(generator.getBoneCount());

			for (size_t i = 0u; i < bones.size(); ++i) {
				bones[i].root = generator.getBoneRoot(i);
				bones[i].localBounds = generator.getBoneBounds(i);
				bones[i].transform = bones[i].root ? bones[i].root->getCoordinateFrame() : CoordinateFrame();
			}

			return vertexCount;
		}

		ClusterStats& FastCluster::getStatsBucket() {
			RenderStats* stats = getVisualEngine()->getRenderStats();

			if (humanoid)
				return stats->clusterFastHumanoid;
			else if (fw)
				return stats->clusterFastFW;
			else
				return stats->clusterFast;
		}

		void FastCluster::updateCoordinateFrame(bool recalcLocalBounds)
		{
			Extents oldWorldBB = getWorldBounds();
			Extents newWorldBB;

			bool bonesChanged = false;

			RBX::Time now = RBX::Time::nowFast();

			for (size_t i = 0u; i < bones.size(); ++i) {
				Bone& bone = bones[i];

				if (bone.root && !bone.root->getSleeping()) {
					CoordinateFrame frame = bone.root->calcRenderingCoordinateFrame(now);

					if (!Math::fuzzyEq(bone.transform, frame))
						bonesChanged = true;

					bone.transform = frame;

					if (owner && owner->getSpatialGrid()->getIndexUnsafe(bone.root, fw ? SpatialGridIndex::fFW : 0u) != owner->getSpatialIndex()) {
						// Bone moved significantly, re-cluster
						queueClusterCheck();
					}
				}

				newWorldBB.expandToContain(bone.localBounds.toWorldSpace(bone.transform));
			}

			updateWorldBounds(newWorldBB);

			/*if (bonesChanged)
			{
				invalidateLighting(oldWorldBB);
				invalidateLighting(newWorldBB);
			}*/
		}

		/*void FastCluster::invalidateLighting(const Extents& bbox) {
			if (humanoid) return;

			if (!bbox.isNull())
				getVisualEngine()->getSceneUpdater()->lightingInvalidateOccupancy(bbox, bbox.center(), fw);
		}*/

		void FastCluster::unbind() {
			FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: unbind (%d parts, %d own connections)", this, parts.size(), connections.size());

			GfxPart::unbind();

			getStatsBucket().parts -= parts.size();

			for (size_t i = 0u; i < parts.size(); ++i) {
				Part& part = parts[i];

				part.binding->unbind();
				delete part.binding;
			}

			bones.clear();
			parts.clear();
		}

		void FastCluster::onSleepingChanged(bool sleeping, PartInstance* part) {
			if (!owner) return;

			// FW parts react on wakeup, non-FW parts react on sleep
			if ((fw && !sleeping) || (!fw && sleeping)) {
				bool isPartFW = SceneUpdater::isPartStatic(part);

				if (isPartFW != fw) {
					FASTLOG5(FLog::RenderFastCluster, "FastCluster[%p]: part %p, sleeping %d (fw %d -> %d)", this, part, sleeping, fw, isPartFW);

					queueClusterCheck();
				}
			}
		}

		void FastCluster::onClumpChanged(PartInstance* part) {
			if (!fw) {
				FASTLOG2(FLog::RenderFastCluster, "FastCluster[%p]: part %p requests clump change", this, part);

				invalidateEntity();
			}
		}

		void FastCluster::queueClusterCheck() {
			if (!owner) return;

			getVisualEngine()->getSceneUpdater()->queueFastClusterCheck(this, isFW());
		}

		void FastCluster::markLightingDirty() {
			lightDirty = true;
		}

		size_t FastCluster::getPartCount() {
			return parts.size();
		}

		const SpatialGridIndex& FastCluster::getSpatialIndex() const {
			return owner->getSpatialIndex();
		}

		void FastCluster::getAllParts(std::vector< shared_ptr<PartInstance> >& retParts) const {
			retParts.resize(parts.size());

			for (size_t j = 0u, e = parts.size(); j < e; ++j)
				retParts[j] = shared_from(parts[j].instance);
		}

	}
}
