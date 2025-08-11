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

namespace RBX {
	namespace Graphics {

		MeshInstanceBinding::MeshInstanceBinding(MeshInstanceSubCluster* subCluster, const shared_ptr<PartInstance>& part, uint32_t index)
			: GfxPart(part)
			, subCluster(subCluster)
			, index(index)
		{
			bindProperties(part);

			RBXASSERT(part->getGfxPart() == nullptr);
			part->setGfxPart(this);
		}

		void MeshInstanceBinding::invalidateEntity() {
			subCluster->updatePart(index, false);
		}

		void MeshInstanceBinding::onCoordinateFrameChanged() {
			transforms.cframe = partInstance->getCoordinateFrame().toMatrix4();

			updateModelMatrix();
		}

		void MeshInstanceBinding::onMaterialChanged() {
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

			instance.Color = Color4(instance.Color.rgb(), transparency);

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

		void MeshInstanceBinding::updateModelMatrix() {
			instance.Model = transforms.scale * transforms.cframe;

			if (flags & RenderQueue::Flag::Flag_Opaque)
				subCluster->updateInstances(0u);

			if (flags & RenderQueue::Flag::Flag_Transparent)
				subCluster->updateInstances(1u);

			if (flags & RenderQueue::Flag::Flag_DepthPrepass)
				subCluster->updateInstances(2u);

			if (flags & RenderQueue::Flag::Flag_ShadowPass)
				subCluster->updateInstances(3u);
		};

		MeshInstanceSubCluster::MeshInstanceSubCluster()
			: cluster(nullptr)
		{
		}

		MeshInstanceSubCluster::MeshInstanceSubCluster(MeshInstanceCluster* cluster, const shared_ptr<Material>& materials)
			: cluster(cluster)
		{
			material[0] = materials;
		}

		void MeshInstanceSubCluster::addPart(const shared_ptr<PartInstance>& part) {
			Part partInstance = {};
			partInstance.instance = part;
			partInstance.binding = new MeshInstanceBinding(this, part, parts.size());

			parts.push_back(partInstance);
		}

		void MeshInstanceSubCluster::updatePart(uint32_t index, bool materialUpdate) {
			Part& instance = parts[index];
			shared_ptr<PartInstance>& partInstance = instance.instance;

			removePart(index);

			cluster->updatePart(partInstance, materialUpdate);
		}

		void MeshInstanceSubCluster::removePart(uint32_t index) {
			Part& instance = parts[index];
			uint8_t flags = instance.binding->getFlags();

			delete instance.binding;
			instance.binding = nullptr;

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
				parts[i].binding->updateIndex(i);
		}

		void MeshInstanceSubCluster::updateInstances(uint8_t index) {
			instances[index].Models.clear();

			for (auto& part : parts)
				instances[index].Models.push_back(part.binding->getInstanceData());
		}

		void MeshInstanceSubCluster::clearSubCluster() {
			for (size_t i = 0u; i < 4u; ++i) {
				material[i].reset();
				instances[i].Models.clear();
			}

			for (auto& part : parts) {
				delete part.binding;
				part.binding = nullptr;
			}

			parts.clear();
		}

		MeshInstanceCluster::MeshInstanceCluster()
			: instancer(nullptr)
		{
		}

		MeshInstanceCluster::MeshInstanceCluster(MeshInstancer* instancer, const GeometryBatch& geometry)
			: instancer(instancer)
			, geometry(geometry)
		{
		}

		void MeshInstanceCluster::addPart(const shared_ptr<PartInstance>& part) {
			MaterialGenerator* materialGenerator = instancer->getMaterialGenerator();
			MaterialGenerator::Result materialResult = materialGenerator->createMaterial(part.get(), nullptr, materialGenerator->createFlags(part.get(), false));

			if (!subClusters.count(materialResult.key))
				subClusters.emplace(std::make_pair(materialResult.key, MeshInstanceSubCluster(this, materialResult.material)));

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

			dataModel->getWorkspace()->visitDescendants(boost::bind(&MeshInstancer::onWorkspaceDescendantAdded, this, _1));
			workspaceDescendantAddedConnection = dataModel->getWorkspace()->onDemandWrite()->descendantAddedSignal.connect(boost::bind(&MeshInstancer::onWorkspaceDescendantAdded, this, _1));

			initialiseBuffers();
		}

		void MeshInstancer::addPart(const shared_ptr<PartInstance>& part) {
			size_t clusterIndex = uint32_t(part->getPartType());

			// Check if geometry cluster exists. If it doesn't, create a new one.
			if (!partClusters.count(clusterIndex)) {
				GeometryGenerator geometryGenerator;

				GeometryGenerator::GeometryPair geometryPair = geometryGenerator.generateGeometry(part.get());

				partClusters.emplace(std::make_pair(clusterIndex, MeshInstanceCluster(this, GeometryBatch(geometry, Geometry::Primitive_Triangles, geometryPair.indices.size(), vertices.size(), indices.size()))));

				std::copy(geometryPair.vertices.begin(), geometryPair.vertices.end(), std::back_inserter(vertices));
				std::copy(geometryPair.indices.begin(), geometryPair.indices.end(), std::back_inserter(indices));

				updateBuffers();
			}

			partClusters[clusterIndex].addPart(part);
		}

		void MeshInstancer::updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, RenderQueue::Flag flag) const {
			// TODO: Find a way to push render operations into the render queue in a material contiguous manner.
			for (const auto& clustersPair : partClusters) {
				auto& subClusters = clustersPair.second.getSubClusters();
				auto& clusterGeometry = clustersPair.second.getGeometry();

				for (const auto& subClusterPair : subClusters) {
					auto& subCluster = subClusterPair.second;

					if (subCluster.isSubClusterActive()) {
						uint8_t index = RenderQueue::getIndex(flag);

						RenderOperation rop = { &clusterGeometry, &subCluster.getMaterial(index).get()->getTechniques()[0], &subCluster.getInstances(index) };

						queue.getGroup(index).push(rop);
					}
				}
			}

			for (const auto& clustersPair : meshClusters) {
				auto& subClusters = clustersPair.second.getSubClusters();
				auto& clusterGeometry = clustersPair.second.getGeometry();

				for (const auto& subClusterPair : subClusters) {
					auto& subCluster = subClusterPair.second;

					if (subCluster.isSubClusterActive()) {
						uint8_t index = RenderQueue::getIndex(flag);

						RenderOperation rop = { &clusterGeometry, &subCluster.getMaterial(index).get()->getTechniques()[0], &subCluster.getInstances(index) };

						queue.getGroup(index).push(rop);
					}
				}
			}
		}

		void MeshInstancer::unbind() {
			workspaceDescendantAddedConnection.disconnect();

			materialGenerator.release();

			for (auto& cluster : partClusters)
				cluster.second.clearCluster();

			for (auto& cluster : meshClusters)
				cluster.second.clearCluster();

			vertices.clear();
			indices.clear();

			geometry.reset();
			vertexBuffer.reset();
			indexBuffer.reset();
		}

		void MeshInstancer::initialiseBuffers() {
			shared_ptr<VertexLayout> layout = device->createVertexLayout(MaterialVertex::getVertexLayout());

			updateBuffers();

			geometry = device->createGeometry(layout, vertexBuffer, indexBuffer);
		}

		// Recreating the buffer seems like a lot for simply updating the vertex and index buffers, but from what I can tell the byte width of a buffer can't be changed post-creation
		void MeshInstancer::updateBuffers() {
			vertexBuffer.reset();
			indexBuffer.reset();

			vertexBuffer = device->createVertexBuffer(sizeof(MaterialVertex), vertices.size(), GeometryBuffer::Usage_Static);
			indexBuffer = device->createIndexBuffer(sizeof(uint32_t), indices.size(), GeometryBuffer::Usage_Static);

			vertexBuffer->upload(0u, vertices.data(), vertices.size());
			indexBuffer->upload(0u, indices.data(), indices.size());
		}

		void MeshInstancer::onWorkspaceDescendantAdded(shared_ptr<RBX::Instance> descendant) {
			// See if the new instance is a PartInstance
			RBX::PartInstance* partInstance = RBX::Instance::fastDynamicCast<RBX::PartInstance>(descendant.get());

			// TODO: Implement buffering to defer part updates to happen at once, rather than immediately whenever a new part is added
			if (partInstance != nullptr)
				addPart(shared_from(partInstance));
		}

	}
}