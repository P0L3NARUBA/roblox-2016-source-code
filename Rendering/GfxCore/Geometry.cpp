#include "GfxCore/Geometry.h"

#include "rbx/Profiler.h"

namespace RBX {
	namespace Graphics {

		VertexLayout::Element::Element(uint32_t stream, uint32_t offset, Format format, Input input, Semantic semantic, uint32_t semanticIndex)
			: stream(stream)
			, offset(offset)
			, format(format)
			, input(input)
			, semantic(semantic)
			, semanticIndex(semanticIndex)
		{
		}

		VertexLayout::VertexLayout(Device* device, const std::vector<Element>& elements)
			: Resource(device)
			, elements(elements)
		{
		}

		VertexLayout::~VertexLayout()
		{
		}

		GeometryBuffer::GeometryBuffer(Device* device, uint32_t elementSize, uint32_t elementCount, Usage usage)
			: Resource(device)
			, elementSize(elementSize)
			, elementCount(elementCount)
			, usage(usage)
		{
		}

		GeometryBuffer::~GeometryBuffer()
		{
		}

		VertexBuffer::VertexBuffer(Device* device, size_t elementSize, size_t elementCount, Usage usage)
			: GeometryBuffer(device, elementSize, elementCount, usage)
		{
			RBXPROFILER_COUNTER_ADD("memory/gpu/vertexbuffer", elementSize * elementCount);
		}

		VertexBuffer::~VertexBuffer() {
			RBXPROFILER_COUNTER_SUB("memory/gpu/vertexbuffer", elementSize * elementCount);
		}

		IndexBuffer::IndexBuffer(Device* device, size_t elementSize, size_t elementCount, Usage usage)
			: GeometryBuffer(device, elementSize, elementCount, usage)
		{
			RBXPROFILER_COUNTER_ADD("memory/gpu/indexbuffer", elementSize * elementCount);
		}

		IndexBuffer::~IndexBuffer() {
			RBXPROFILER_COUNTER_SUB("memory/gpu/indexbuffer", elementSize * elementCount);
		}

		Geometry::Geometry(Device* device, const shared_ptr<VertexLayout>& layout, const std::vector<shared_ptr<VertexBuffer> >& vertexBuffers, const shared_ptr<IndexBuffer>& indexBuffer, uint32_t baseVertexIndex)
			: Resource(device)
			, layout(layout)
			, vertexBuffers(vertexBuffers)
			, indexBuffer(indexBuffer)
			, baseVertexIndex(baseVertexIndex)
		{
		}

		Geometry::~Geometry()
		{
		}

		inline bool isCountValid(Geometry::Primitive primitive, uint32_t count) {
			switch (primitive) {
			case Geometry::Primitive_Triangles: return count % 3u == 0u;
			case Geometry::Primitive_Lines: return count % 2u == 0u;
			case Geometry::Primitive_Points: return true;
			case Geometry::Primitive_TriangleStrip: return count != 1u && count != 2u;
			default: return false;
			}
		}

		GeometryBatch::GeometryBatch(const shared_ptr<Geometry>& geometry, Geometry::Primitive primitive, uint32_t count, uint32_t indexRangeSize)
			: geometry(geometry)
			, primitive(primitive)
			, offset(0u)
			, count(count)
			, indexRangeBegin(0u)
			, indexRangeEnd(indexRangeSize)
		{
			RBXASSERT(geometry && isCountValid(primitive, count));
		}

		GeometryBatch::GeometryBatch(const shared_ptr<Geometry>& geometry, Geometry::Primitive primitive, uint32_t offset, uint32_t count, uint32_t indexRangeBegin, uint32_t indexRangeEnd)
			: geometry(geometry)
			, primitive(primitive)
			, offset(offset)
			, count(count)
			, indexRangeBegin(indexRangeBegin)
			, indexRangeEnd(indexRangeEnd)
		{
			RBXASSERT(geometry && isCountValid(primitive, count) && indexRangeBegin <= indexRangeEnd);
		}

	}
}
