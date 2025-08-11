#pragma once

#include "util/G3DCore.h"
#include "GlobalShaderData.h"

#include <unordered_map>

namespace RBX {
	namespace Graphics {

		class GeometryBatch;
		class Technique;
		struct InstancedModels;

		class Renderable {
		public:
			virtual ~Renderable();

			//virtual Matrix4 getWorldTransforms(float* buffer, const void** cacheKey) const = 0;

		protected:
			static bool useCache(const void** cacheKey, const void* value) {
				if (*cacheKey == value) return true;
				*cacheKey = value;
				return false;
			}
		};

		struct RenderOperation {
			const GeometryBatch* geometry;
			const Technique* technique;
			const InstancedModels* models;
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

			enum Flag {
				Flag_Opaque = 1u << 0u,
				Flag_Transparent = 1u << 1u,
				Flag_DepthPrepass = 1u << 2u,
				Flag_ShadowPass = 1u << 3u,

				Flag_Max = (Flag_Opaque | Flag_Transparent | Flag_DepthPrepass | Flag_ShadowPass) + 1u,
			};

			RenderQueue();

			void clear();

			RenderQueueGroup& getGroup(uint8_t flag) {
				return groups[flag];
			}

			static uint8_t getIndex(Flag flag) {
				switch (flag) {
				default:
				case (Flag_Opaque):
					return 0u;
				case (Flag_Transparent):
					return 1u;
				case (Flag_DepthPrepass):
					return 2u;
				case (Flag_ShadowPass):
					return 3u;
				}
			};

			uint32_t getFeatures() const { return features; }
			void setFeature(uint32_t feature) { features |= feature; }

		private:
			RenderQueueGroup groups[4];
			uint32_t features;
		};

	}
}
