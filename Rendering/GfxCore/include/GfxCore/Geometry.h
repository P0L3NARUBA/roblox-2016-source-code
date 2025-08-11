#pragma once

#include "Resource.h"

#include <vector>

namespace RBX {
	namespace Graphics {

		class VertexLayout : public Resource {
		public:
			enum Semantic {
				Semantic_Position,
				Semantic_Texture,
				Semantic_Color,
				Semantic_Tangent,
				Semantic_Normal,

				Semantic_Count
			};

			enum Format {
				Format_UInt1,	/* 32-bit unsigned integer x 1, 4 bytes */
				Format_Float1,	/* 32-bit signed float x 1, 4 bytes */
				Format_Float2,	/* 32-bit signed float x 2, 8 bytes */
				Format_Float3,	/* 32-bit signed float x 3, 12 bytes */
				Format_Float4,	/* 32-bit signed float x 4, 16 bytes */
				Format_Short2,	/* 16-bit signed float x 2, 4 bytes */
				Format_Short4,	/* 16-bit signed float x 4, 8 bytes */
				Format_UByte4,	/* 8-bit unsigned integer x 4, 4 bytes */
				Format_Color,	/* 8-bit normalized unsigned integer x 4, 4 bytes */

				Format_Count	/* Do not use */
			};

			enum Input {
				Input_Vertex,
				Input_Instance,

				Input_Count
			};

			struct Element {
				Semantic semanticName;
				uint32_t semanticIndex;

				Format format;
				uint32_t offset;

				Input inputClass;
				uint32_t stepRate;

				Element(Semantic semanticName, uint32_t semanticIndex, Format format, uint32_t offset, Input inputClass, uint32_t stepRate = 0u);
			};

			~VertexLayout();

			const std::vector<Element>& getElements() const { return elements; }

		protected:
			VertexLayout(Device* device, const std::vector<Element>& elements);

			std::vector<Element> elements;
		};

		class GeometryBuffer : public Resource {
		public:
			enum Usage {
				Usage_Static,
				Usage_Dynamic,

				Usage_Count
			};

			enum LockMode {
				Lock_Normal,
				Lock_Discard,

				Lock_Count
			};

			~GeometryBuffer();

			virtual void* lock(LockMode mode = Lock_Normal) = 0;
			virtual void unlock() = 0;

			virtual void upload(uint32_t offset, const void* data, size_t size) = 0;

			size_t getElementSize() const { return elementSize; }
			size_t getElementCount() const { return elementCount; }

		protected:
			GeometryBuffer(Device* device, uint32_t elementSize, uint32_t elementCount, Usage usage);

			uint32_t elementSize;
			uint32_t elementCount;
			Usage usage;
		};

		class VertexBuffer : public GeometryBuffer {
		public:
			~VertexBuffer();

		protected:
			VertexBuffer(Device* device, size_t elementSize, size_t elementCount, Usage usage);
		};

		class IndexBuffer : public GeometryBuffer {
		public:
			~IndexBuffer();

		protected:
			IndexBuffer(Device* device, size_t elementSize, size_t elementCount, Usage usage);
		};

		class Geometry : public Resource
		{
		public:
			enum Primitive {
				Primitive_Triangles,
				Primitive_Lines,
				Primitive_Points,
				Primitive_TriangleStrip,

				Primitive_Count
			};

			~Geometry();

			/*void updateBuffers(const std::shared_ptr<VertexBuffer>& newVertexBuffer, const std::shared_ptr<IndexBuffer>& newIndexbuffer) {
				vertexBuffer = newVertexBuffer;
				indexBuffer = newIndexbuffer;
			};*/

		protected:
			Geometry(Device* device, const shared_ptr<VertexLayout>& layout, const shared_ptr<VertexBuffer>& vertexBuffer, const shared_ptr<IndexBuffer>& indexBuffer);

			shared_ptr<VertexLayout> layout;
			shared_ptr<VertexBuffer> vertexBuffer;
			shared_ptr<IndexBuffer> indexBuffer;
		};

		class GeometryBatch {
		public:
			GeometryBatch();
			GeometryBatch(const shared_ptr<Geometry>& geometry, Geometry::Primitive primitive, uint32_t vertexCount, uint32_t vertexOffset);
			GeometryBatch(const shared_ptr<Geometry>& geometry, Geometry::Primitive primitive, uint32_t vertexCount, uint32_t vertexOffset, uint32_t indexOffset);

			Geometry* getGeometry() const { return geometry.get(); }
			Geometry::Primitive getPrimitive() const { return primitive; }

			uint32_t getVertexCount() const { return vertexCount; }
			uint32_t getVerexOffset() const { return vertexOffset; }
			uint32_t getIndexOffset() const { return indexOffset; }

		private:
			shared_ptr<Geometry> geometry;

			Geometry::Primitive primitive;

			uint32_t vertexCount;
			uint32_t vertexOffset;
			uint32_t indexOffset;
		};
		
	}
}
