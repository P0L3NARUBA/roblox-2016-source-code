#include "stdafx.h"
#include "MeshInstancer.h"

#include "SceneManager.h"
#include "GeometryGenerator.h"
#include "MaterialGenerator.h"
#include "Util.h"
#include "SceneUpdater.h"
#include "Material.h"

#include "v8datamodel/DataModel.h"
#include "v8datamodel/Workspace.h"

#include "VisualEngine.h"

#include "rbx/Profiler.h"

namespace RBX {
	namespace Graphics {

		MeshInstanceBinding::MeshInstanceBinding(MeshInstanceSubCluster* subCluster, const shared_ptr<PartInstance>& part, uint32_t index)
			: GfxPart(part)
			, subCluster(subCluster)
			, index(index)
		{
			bindProperties(part);

			part->setGfxPart(this);

			Vector3 scaleVector = partInstance->getPartSizeUi();
			Matrix4 scaleMatrix = Matrix4::identity();
			scaleMatrix[0][0] = scaleVector.x;
			scaleMatrix[1][1] = scaleVector.y;
			scaleMatrix[2][2] = scaleVector.z;

			transforms.scale = scaleMatrix;
			transforms.cframe = partInstance->getCoordinateFrame().toMatrix4();

			instance.Model = transforms.scale * transforms.cframe;
			instance.Color = Color4(partInstance->getColor(), 1.0f);
			instance.MaterialID = part->getRenderMaterial();

			flags = RenderQueue::Flag_None;

			onTransparencyChanged();
		}

		void MeshInstanceBinding::invalidateEntity() {
			subCluster->updatePart(index, false);
		}

		void MeshInstanceBinding::onCoordinateFrameChanged() {
			transforms.cframe = partInstance->getCoordinateFrame().toMatrix4();

			updateModelMatrix();
		}

		void MeshInstanceBinding::onMaterialChanged() {
			instance.MaterialID = partInstance->getRenderMaterial();

			MaterialGenerator* materialGenerator = subCluster->getCluster()->getInstancer()->getMaterialGenerator();
			MaterialGenerator::Result materialResult = materialGenerator->createMaterial(partInstance.get(), nullptr, materialGenerator->createFlags(partInstance.get(), false));

			if (subCluster->getMaterialKey() != materialResult.key)
				subCluster->updatePart(index, true);
		}

		void MeshInstanceBinding::onColorChanged() {
			instance.Color = Color4(partInstance->getColor(), instance.Color.a);

			if (flags & RenderQueue::Flag::Flag_Opaque)
				subCluster->updateInstances(0u);

			if (flags & RenderQueue::Flag::Flag_Transparent)
				subCluster->updateInstances(1u);
		}

		void MeshInstanceBinding::onSizeChanged() {
			Vector3 scaleVector = partInstance->getPartSizeUi();
			Matrix4 scaleMatrix = Matrix4::identity();
			scaleMatrix[0][0] = scaleVector.x;
			scaleMatrix[1][1] = scaleVector.y;
			scaleMatrix[2][2] = scaleVector.z;

			transforms.scale = scaleMatrix;

			updateModelMatrix();
		}

		void MeshInstanceBinding::onTransparencyChanged() {
			float transparency = std::max(partInstance->getTransparencyUi(), 0.0f);

			instance.Color = Color4(instance.Color.rgb(), 1.0f - transparency);

			if (transparency < FLT_EPSILON && !(flags & RenderQueue::Flag::Flag_Opaque)) {
				flags |= RenderQueue::Flag::Flag_Opaque;
				subCluster->updateInstances(0u);

				flags ^= RenderQueue::Flag::Flag_Transparent;
				subCluster->updateInstances(1u);

				if (subCluster->getDepthPrepassEnabled()) {
					flags |= RenderQueue::Flag::Flag_DepthPrepass;
					subCluster->updateInstances(2u);
				}
			}
			else if (transparency < 1.0f && !(flags & RenderQueue::Flag::Flag_Transparent)) {
				flags ^= RenderQueue::Flag::Flag_Opaque;
				subCluster->updateInstances(0u);

				flags |= RenderQueue::Flag::Flag_Transparent;
				subCluster->updateInstances(1u);

				if (flags & RenderQueue::Flag::Flag_DepthPrepass) {
					flags ^= RenderQueue::Flag::Flag_DepthPrepass;
					subCluster->updateInstances(2u);
				}
			}
			else {
				if (flags & RenderQueue::Flag::Flag_Opaque)
					subCluster->updateInstances(0u);

				if (flags & RenderQueue::Flag::Flag_Transparent)
					subCluster->updateInstances(1u);

				if (flags & RenderQueue::Flag::Flag_DepthPrepass)
					subCluster->updateInstances(2u);

				flags &= RenderQueue::Flag::Flag_ShadowPass;
			}
		}

		void MeshInstanceBinding::unbind() {
			GfxPart::unbind();

			subCluster->removePart(index);
		}

		void MeshInstanceBinding::updateInstances() {
			if (flags & RenderQueue::Flag::Flag_Opaque)
				subCluster->updateInstances(0u);

			if (flags & RenderQueue::Flag::Flag_Transparent)
				subCluster->updateInstances(1u);

			if (flags & RenderQueue::Flag::Flag_DepthPrepass)
				subCluster->updateInstances(2u);

			if (flags & RenderQueue::Flag::Flag_ShadowPass)
				subCluster->updateInstances(3u);
		}

		void MeshInstanceBinding::updateModelMatrix() {
			instance.Model = transforms.scale * transforms.cframe;

			updateInstances();
		};

		MeshInstanceSubCluster::MeshInstanceSubCluster()
			: cluster(nullptr)
		{
		}

		MeshInstanceSubCluster::MeshInstanceSubCluster(MeshInstanceCluster* cluster, uint32_t materialKey, const std::shared_ptr<Material>& materials)
			: cluster(cluster)
			, materialKey(materialKey)
		{
			material[0] = materials;

			for (auto& instance : instances)
				instance.reset(new InstancedModels());
		}

		void MeshInstanceSubCluster::addPart(const shared_ptr<PartInstance>& part) {
			Part partInstance = {
				part,
				MeshInstanceBinding(this, part, parts.size())
			};

			parts.push_back(partInstance);
		}

		void MeshInstanceSubCluster::updatePart(uint32_t index, bool materialUpdate) {
			Part& instance = parts[index];
			shared_ptr<PartInstance>& partInstance = instance.instance;

			instance.binding.unbind();

			cluster->updatePart(partInstance, materialUpdate);
		}

		void MeshInstanceSubCluster::removePart(uint32_t index) {
			Part& instance = parts[index];
			uint8_t flags = instance.binding.getFlags();

			parts.erase(parts.begin() + index);

			updatePartIndexes();

			if (flags & RenderQueue::Flag::Flag_Opaque)
				updateInstances(0u);

			if (flags & RenderQueue::Flag::Flag_Transparent)
				updateInstances(1u);

			if (flags & RenderQueue::Flag::Flag_DepthPrepass)
				updateInstances(2u);

			if (flags & RenderQueue::Flag::Flag_ShadowPass)
				updateInstances(3u);
		}

		void MeshInstanceSubCluster::sortInstances(uint8_t index, Vector3 cameraPosition, bool reversed) {
			// TODO: Figure out how to sort by distance to camera efficiently.
		}

		void MeshInstanceSubCluster::updatePartIndexes() {
			// TODO: Find a better method than to manually store index values in each instance binding. May not be necessary, though.
			for (size_t i = 0u; i < parts.size(); ++i)
				parts[i].binding.updateIndex(i);
		}

		void MeshInstanceSubCluster::updateInstances(uint8_t index) {
			instances[index]->Models.clear();

			for (auto& part : parts)
				instances[index]->Models.push_back(part.binding.getInstanceData());
		}

		void MeshInstanceSubCluster::clearSubCluster() {
			for (size_t i = 0u; i < 4u; ++i) {
				material[i].reset();
				instances[i].reset();
			}

			parts.clear();
		}

		MeshInstanceCluster::MeshInstanceCluster()
			: instancer(nullptr)
		{
		}

		MeshInstanceCluster::MeshInstanceCluster(MeshInstancer* instancer, const std::shared_ptr<Geometry>& geometry, uint32_t indexCount, uint32_t vertexOffset, uint32_t indexOffset)
			: instancer(instancer)
		{
			geometryBatch.reset(new GeometryBatch(geometry, Geometry::Primitive_Triangles, indexCount, vertexOffset, indexOffset));
		}

		void MeshInstanceCluster::addPart(const shared_ptr<PartInstance>& part) {
			MaterialGenerator* materialGenerator = instancer->getMaterialGenerator();
			MaterialGenerator::Result materialResult = materialGenerator->createMaterial(part.get(), nullptr, materialGenerator->createFlags(part.get(), false));

			if (!subClusters.count(materialResult.key))
				subClusters.emplace(std::make_pair(materialResult.key, MeshInstanceSubCluster(this, materialResult.key, materialResult.material)));

			subClusters[materialResult.key].addPart(part);
		}

		void MeshInstanceCluster::updatePart(const shared_ptr<PartInstance>& part, bool materialUpdate) {
			if (materialUpdate)
				addPart(part);
			else
				instancer->addPart(part);
		}

		void MeshInstanceCluster::clearCluster() {
			for (auto& subCluster : subClusters)
				subCluster.second.clearSubCluster();
		}

		MeshInstancer::MeshInstancer(shared_ptr<RBX::DataModel> dataModel, VisualEngine* visualEngine)
			: dataModel(dataModel)
			, visualEngine(visualEngine)
			, device(visualEngine->getDevice())
		{
			materialGenerator.reset(new MaterialGenerator(visualEngine));

			initialiseBuffers();

			dataModel->getWorkspace()->visitDescendants(boost::bind(&MeshInstancer::onWorkspaceDescendantAdded, this, _1));
			workspaceDescendantAddedConnection = dataModel->getWorkspace()->onDemandWrite()->descendantAddedSignal.connect(boost::bind(&MeshInstancer::onWorkspaceDescendantAdded, this, _1));
		}

		void MeshInstancer::addPart(const shared_ptr<PartInstance>& part) {
			size_t clusterIndex = static_cast<uint32_t>(part->getPartType());

			// Check if geometry cluster exists. If it doesn't, create a new one.
			if (!partClusters.count(clusterIndex)) {
				GeometryGenerator::GeometryPair geometryPair = GeometryGenerator::generateGeometry(part.get());

				if (geometryPair.vertices.size() == 0u || geometryPair.indices.size() == 0u)
					return;

				partClusters.emplace(std::make_pair(clusterIndex, MeshInstanceCluster(this, geometry, geometryPair.indices.size(), vertices.size(), indices.size())));

				std::copy(geometryPair.vertices.begin(), geometryPair.vertices.end(), std::back_inserter(vertices));
				std::copy(geometryPair.indices.begin(), geometryPair.indices.end(), std::back_inserter(indices));

				updateBuffers();
			}

			partClusters[clusterIndex].addPart(part);
		}

		void MeshInstancer::updateClusters() {
			RBXPROFILER_SCOPE("Render", "UpdateClusters");

			PartInstanceSet localCopy;

			{
				RBX::mutex::scoped_lock scoped_lock(queue_mutex);

				localCopy.swap(pendingParts);
			}

			for (PartInstanceSet::iterator it = localCopy.begin(); it != localCopy.end(); ++it) {
				shared_ptr<PartInstance> partInstance = it->second.lock();

				if (partInstance && GfxPart::isInWorkspace(partInstance.get()))
					addPart(partInstance);
			}
		}

		void MeshInstancer::updateRenderQueue(RenderQueue& queue, const RenderCamera& camera) {
			// TODO: Find a way to push render operations into the render queue in a material contiguous manner.
			for (const auto& clustersPair : partClusters) {
				const auto& subClusters = clustersPair.second.getSubClusters();
				const auto& clusterGeometry = clustersPair.second.getGeometry();

				for (const auto& subClusterPair : subClusters) {
					const auto& subCluster = subClusterPair.second;

					for (size_t i = 0u; i < 1u; ++i)
						if (subCluster.isSubClusterActive(i))
							queue.getGroup(i).push(RenderOperation(clusterGeometry, &subCluster.getMaterial(i).get()->getTechniques()[0], subCluster.getInstances(i)));
				}
			}

			for (const auto& clustersPair : meshClusters) {
				const auto& subClusters = clustersPair.second.getSubClusters();
				const auto& clusterGeometry = clustersPair.second.getGeometry();

				for (const auto& subClusterPair : subClusters) {
					const auto& subCluster = subClusterPair.second;

					for (size_t i = 0u; i < 1u; ++i)
						if (subCluster.isSubClusterActive(i))
							queue.getGroup(i).push(RenderOperation(clusterGeometry, &subCluster.getMaterial(i).get()->getTechniques()[0], subCluster.getInstances(i)));
				}
			}
		}

		void MeshInstancer::unbind() {
			workspaceDescendantAddedConnection.disconnect();

			materialGenerator.reset();

			for (auto& cluster : partClusters)
				cluster.second.clearCluster();

			for (auto& cluster : meshClusters)
				cluster.second.clearCluster();

			vertices.clear();
			indices.clear();

			geometry.reset();
		}

		void MeshInstancer::initialiseBuffers() {
			geometry = device->createGeometry(
				device->createVertexLayout(MaterialVertex::getVertexLayout()),
				device->createVertexBuffer(sizeof(MaterialVertex), 3u, GeometryBuffer::Usage_Static),
				device->createIndexBuffer(sizeof(uint32_t), 3u, GeometryBuffer::Usage_Static));
		}

		void MeshInstancer::updateBuffers() {
			std::shared_ptr<VertexBuffer> vertexBuffer = device->createVertexBuffer(sizeof(MaterialVertex), vertices.size(), GeometryBuffer::Usage_Static);
			std::shared_ptr<IndexBuffer> indexBuffer = device->createIndexBuffer(sizeof(uint32_t), indices.size(), GeometryBuffer::Usage_Static);

			vertexBuffer->upload(0u, vertices.data(), vertices.size() * sizeof(MaterialVertex));
			indexBuffer->upload(0u, indices.data(), vertices.size() * sizeof(uint32_t));

			geometry->updateBuffers(vertexBuffer, indexBuffer);
		}

		void MeshInstancer::onWorkspaceDescendantAdded(shared_ptr<RBX::Instance> descendant) {
			// See if the new instance is a PartInstance
			RBX::PartInstance* partInstance = RBX::Instance::fastDynamicCast<RBX::PartInstance>(descendant.get());

			if (partInstance != nullptr) {
				RBX::mutex::scoped_lock scoped_lock(queue_mutex);

				pendingParts[partInstance] = shared_from(partInstance);
			}
		}

	}
}