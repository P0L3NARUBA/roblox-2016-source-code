#pragma once

#include <vector>
#include <cstdlib>

namespace RBX {
	namespace Graphics {

		class GeometryBatch;
		class Technique;

		class Renderable {
		public:
			virtual ~Renderable();

			virtual uint32_t getWorldTransforms4x3(float* buffer, uint32_t maxTransforms, const void** cacheKey) const = 0;

		protected:
			static bool useCache(const void** cacheKey, const void* value) {
				if (*cacheKey == value) return true;
				*cacheKey = value;
				return false;
			}
		};

		struct RenderOperation {
			const Renderable* renderable;
			float distanceKey;

			const Technique* technique;
			const GeometryBatch* geometry;
		};

		class RenderQueueGroup {
		public:
			enum SortMode
			{
				Sort_None,
				Sort_Material,
				Sort_Distance
			};

			void clear();

			void sort(SortMode mode);

			void push(const RenderOperation& op) {
				operations.push_back(op);
			}

			const RenderOperation& operator[](uint32_t index) const {
				return operations[index];
			}

			size_t size() const {
				return operations.size();
			}

		private:
			std::vector<RenderOperation> operations;
		};

		class RenderQueue {
		public:
			enum Pass {
				Pass_Default,
				Pass_Shadows
			};

			enum Id {
				Id_Opaque,
				Id_OpaqueDecals,
				Id_TransparentDecals,
				Id_Decals,
				Id_OpaqueCasters,
				Id_TransparentUnsorted,
				Id_Transparent,
				Id_TransparentCasters,

				Id_Count
			};

			enum Features {
				Features_Glow = 1 << 0
			};

			RenderQueue();

			void clear();

			RenderQueueGroup& getGroup(Id id) {
				return groups[id];
			}

			uint32_t getFeatures() const { return features; }
			void setFeature(uint32_t feature) { features |= feature; }

		private:
			RenderQueueGroup groups[Id_Count];
			uint32_t features;
		};

	}
}
