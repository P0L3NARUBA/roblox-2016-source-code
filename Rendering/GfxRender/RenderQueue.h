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
			RenderOperation(const GeometryBatch& geometry, Technique* technique, std::shared_ptr<InstancedModels> models)
				: geometry(geometry)
				, technique(technique)
				, models(models)
			{
			};

			const GeometryBatch& geometry;
			const Technique* technique;
			const std::shared_ptr<InstancedModels> models;
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
				Flag_None = 0u,
				Flag_Opaque = 1u << 0u,
				Flag_Transparent = 1u << 1u,
				Flag_DepthPrepass = 1u << 2u,
				Flag_ShadowPass = 1u << 3u,
			};

			RenderQueue();

			void clear();

			RenderQueueGroup& getGroup(Flag flag) {
				return groups[getIndex(flag)];
			}

			RenderQueueGroup& getGroup(uint8_t index) {
				return groups[index];
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
			std::array<RenderQueueGroup, 4u> groups;
			uint32_t features;
		};

	}
}
