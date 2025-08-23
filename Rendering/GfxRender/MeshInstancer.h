#pragma once

#include "GfxBase/GfxPart.h"
#include "RenderQueue.h"
#include "Material.h"
#include "Vertex.h"

#include "GfxCore/Device.h"

#include <unordered_map>

namespace RBX {
	class PartInstance;
}

namespace RBX {
	namespace Graphics {

		class VisualEngine;
		class MaterialGenerator;

		class MeshInstanceSubCluster;
		class MeshInstanceCluster;
		class MeshInstancer;

		typedef std::unordered_map<uint32_t, MeshInstanceCluster> Clusters;

		class MeshInstanceBinding : public GfxPart {
		public:
			MeshInstanceBinding(MeshInstanceSubCluster* subCluster, const shared_ptr<PartInstance>& part, uint32_t index);

			virtual void invalidateEntity();
			virtual void onCoordinateFrameChanged();
			virtual void onMaterialChanged();
			virtual void onColorChanged();
			virtual void onSizeChanged();
			virtual void onTransparencyChanged();
			virtual void unbind();

			void updateIndex(uint32_t newIndex) { index = newIndex; };

			uint8_t getFlags() const { return flags; };
			InstancedModel getInstanceData() const { return instance; };

		private:
			void updateInstances();
			void updateModelMatrix();

		private:
			struct Transforms {
				Matrix4 scale;
				Matrix4 cframe;
			};

			Transforms transforms;

			InstancedModel instance;
			MeshInstanceSubCluster* subCluster;

			uint8_t flags;
			uint32_t index;
		};

		class MeshInstanceSubCluster {
		public:
			MeshInstanceSubCluster();
			MeshInstanceSubCluster(MeshInstanceCluster* cluster, uint32_t materialKey, const std::shared_ptr<Material>& materials);

			// Add a new instance binding
			void addPart(const shared_ptr<PartInstance>& part);
			// Handle instance material or mesh updates
			void updatePart(uint32_t index, bool materialUpdate);
			// Remove instance binding at index
			void removePart(uint32_t index);

			// Front to back sorting by default. Use reversed for transparent instance sorting.
			void sortInstances(uint8_t index, Vector3 cameraPosition, bool reversed);

			std::shared_ptr<Material> getMaterial(uint8_t index) const { return material[index]; };
			std::shared_ptr<InstancedModels> getInstances(uint8_t index) const { return instances[index]; };
			uint32_t getMaterialKey() const { return materialKey; };
			MeshInstanceCluster* getCluster() { return cluster; };
			bool getDepthPrepassEnabled() const { return depthPrepassEnabled; };

			void updateInstances(uint8_t index);
			void clearSubCluster();

			bool isSubClusterActive(uint8_t index) const { return instances[index]->Models.size() != 0u; };

		private:
			void updatePartIndexes();

		private:
			MeshInstanceCluster* cluster;

			struct Part {
				shared_ptr<PartInstance> instance;
				MeshInstanceBinding binding;
			};

			std::vector<Part> parts;

			bool depthPrepassEnabled;
			
			uint32_t materialKey;
			std::array<std::shared_ptr<Material>, 4u> material;
			std::array< std::shared_ptr<InstancedModels>, 4u> instances;

		};

		class MeshInstanceCluster {
		public:
			MeshInstanceCluster();
			MeshInstanceCluster(MeshInstancer* instancer, const std::shared_ptr<Geometry>& geometry, uint32_t indexCount, uint32_t vertexOffset, uint32_t indexOffset);

			// Add a new instance binding
			void addPart(const shared_ptr<PartInstance>& part);
			// Handle instance material or mesh updates
			void updatePart(const shared_ptr<PartInstance>& part, bool materialUpdate);

			std::unordered_map<uint8_t, MeshInstanceSubCluster> getSubClusters() const { return subClusters; };
			const GeometryBatch& getGeometry() const { return *geometryBatch; };
			MeshInstancer* getInstancer() { return instancer; };

			void clearCluster();

		private:
			MeshInstancer* instancer;

			// Index by material
			std::unordered_map<uint8_t, MeshInstanceSubCluster> subClusters;

			std::shared_ptr<GeometryBatch> geometryBatch;

		};

		class MeshInstancer {
		public:
			MeshInstancer(shared_ptr<RBX::DataModel> dataModel, VisualEngine* visualEngine);

			void addPart(const shared_ptr<PartInstance>& part);
			void updateClusters();

			void updateRenderQueue(RenderQueue& queue, const RenderCamera& camera);

			MaterialGenerator* getMaterialGenerator() { return materialGenerator.get(); };

			void unbind();

		private:
			void initialiseBuffers();
			void updateBuffers();

			void onWorkspaceDescendantAdded(shared_ptr<RBX::Instance> descendant);

		private:
			VisualEngine* visualEngine;
			Device* device;

			RBX::mutex queue_mutex;

			shared_ptr<RBX::DataModel> dataModel;

			rbx::signals::scoped_connection workspaceDescendantAddedConnection;

			std::unique_ptr<MaterialGenerator> materialGenerator;

			// Regular parts and part operations
			Clusters partClusters;
			// Custom meshes
			Clusters meshClusters;

			typedef std::unordered_map<RBX::PartInstance*, weak_ptr<RBX::PartInstance>> PartInstanceSet;

			PartInstanceSet pendingParts;

			std::vector<MaterialVertex> vertices;
			std::vector<uint32_t> indices;
			// If we ever need to handle more than 1.43 million triangles in a single mesh, we can always use uint64_t for a small 6.14 quintillion triangles

			std::shared_ptr<Geometry> geometry;

		};
	}
}