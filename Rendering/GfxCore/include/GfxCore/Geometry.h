#pragma once

#include "Resource.h"

#include <vector>

namespace RBX
{
	namespace Graphics
	{

		class VertexLayout : public Resource
		{
		public:
			enum Semantic
			{
				Semantic_Position,
				Semantic_Tangent,
				Semantic_Bitangent,
				Semantic_Normal,
				Semantic_Color,
				Semantic_Texture,
				Semantic_InstanceID,

				Semantic_Count
			};

			enum Format
			{
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

			enum Input
			{
				Input_Vertex,
				Input_Instance,

				Input_Count
			};

			struct Element
			{
				unsigned int stream;
				unsigned int offset;

				Format format;
				Input input;
				Semantic semantic;
				unsigned int semanticIndex;

				Element(unsigned int stream, unsigned int offset, Format format, Input input, Semantic semantic, unsigned int semanticIndex = 0);
			};

			~VertexLayout();

			const std::vector<Element>& getElements() const { return elements; }

		protected:
			VertexLayout(Device* device, const std::vector<Element>& elements);

			std::vector<Element> elements;
		};

		class GeometryBuffer : public Resource
		{
		public:
			enum Usage
			{
				Usage_Static,
				Usage_Dynamic,

				Usage_Count
			};

			enum LockMode
			{
				Lock_Normal,
				Lock_Discard,

				Lock_Count
			};

			~GeometryBuffer();

			virtual void* lock(LockMode mode = Lock_Normal) = 0;
			virtual void unlock() = 0;

			virtual void upload(unsigned int offset, const void* data, unsigned int size) = 0;

			size_t getElementSize() const { return elementSize; }
			size_t getElementCount() const { return elementCount; }

		protected:
			GeometryBuffer(Device* device, size_t elementSize, size_t elementCount, Usage usage);

			size_t elementSize;
			size_t elementCount;
			Usage usage;
		};

		class VertexBuffer : public GeometryBuffer
		{
		public:
			~VertexBuffer();

		protected:
			VertexBuffer(Device* device, size_t elementSize, size_t elementCount, Usage usage);
		};

		class IndexBuffer : public GeometryBuffer
		{
		public:
			~IndexBuffer();

		protected:
			IndexBuffer(Device* device, size_t elementSize, size_t elementCount, Usage usage);
		};

		class Geometry : public Resource
		{
		public:
			enum Primitive
			{
				Primitive_Triangles,
				Primitive_Lines,
				Primitive_Points,
				Primitive_TriangleStrip,

				Primitive_Count
			};

			~Geometry();

		protected:
			Geometry(Device* device, const shared_ptr<VertexLayout>& layout, const std::vector<shared_ptr<VertexBuffer> >& vertexBuffers, const shared_ptr<IndexBuffer>& indexBuffer, unsigned int baseVertexIndex);

			shared_ptr<VertexLayout> layout;
			std::vector<shared_ptr<VertexBuffer>> vertexBuffers;
			shared_ptr<IndexBuffer> indexBuffer;
			unsigned int baseVertexIndex;
		};

		class GeometryBatch
		{
		public:
			GeometryBatch(const shared_ptr<Geometry>& geometry, Geometry::Primitive primitive, unsigned int count, unsigned int indexRangeSize);
			GeometryBatch(const shared_ptr<Geometry>& geometry, Geometry::Primitive primitive, unsigned int offset, unsigned int count, unsigned int indexRangeBegin, unsigned int indexRangeEnd);

			Geometry* getGeometry() const { return geometry.get(); }

			Geometry::Primitive getPrimitive() const { return primitive; }
			unsigned int getOffset() const { return offset; }
			unsigned int getCount() const { return count; }

			unsigned int getIndexRangeBegin() const { return indexRangeBegin; }
			unsigned int getIndexRangeEnd() const { return indexRangeEnd; }

		private:
			shared_ptr<Geometry> geometry;

			Geometry::Primitive primitive;
			unsigned int offset;
			unsigned int count;

			unsigned int indexRangeBegin;
			unsigned int indexRangeEnd;
		};

	}
}
