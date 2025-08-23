#include "GfxCore/Geometry.h"

#include "rbx/Profiler.h"

namespace RBX {
	namespace Graphics {

		VertexLayout::Element::Element(Semantic semanticName, uint32_t semanticIndex, Format format, uint32_t offset, Input inputClass, uint32_t stepRate)
			: semanticName(semanticName)
			, semanticIndex(semanticIndex)
			, format(format)
			, offset(offset)
			, inputClass(inputClass)
			, stepRate(stepRate)
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
			, dataSize(0u)
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

		Geometry::Geometry(Device* device, const std::shared_ptr<VertexLayout>& layout, const std::shared_ptr<VertexBuffer>& vertexBuffer, const std::shared_ptr<IndexBuffer>& indexBuffer)
			: Resource(device)
			, layout(layout)
			, vertexBuffer(vertexBuffer)
			, indexBuffer(indexBuffer)
		{
		}

		Geometry::~Geometry()
		{
		}

		inline static bool isCountValid(Geometry::Primitive primitive, uint32_t count) {
			switch (primitive) {
			case Geometry::Primitive_Triangles: return count % 3u == 0u && count >= 3u;
			case Geometry::Primitive_Lines: return count % 2u == 0u && count >= 2u;
			case Geometry::Primitive_Points: return true;
			case Geometry::Primitive_TriangleStrip: return count >= 3u;
			default: return false;
			}
		}

		GeometryBatch::GeometryBatch()
			: geometry(nullptr)
			, primitive(Geometry::Primitive_Triangles)
			, vertexCount(0u)
			, vertexOffset(0u)
			, indexOffset(0u)
		{
		}

		GeometryBatch::GeometryBatch(const std::shared_ptr<Geometry>& geometry, Geometry::Primitive primitive, uint32_t vertexCount, uint32_t vertexOffset)
			: geometry(geometry)
			, primitive(primitive)
			, vertexCount(vertexCount)
			, vertexOffset(vertexOffset)
			, indexOffset(0u)
		{
			RBXASSERT(geometry && isCountValid(primitive, vertexCount));
		}

		GeometryBatch::GeometryBatch(const std::shared_ptr<Geometry>& geometry, Geometry::Primitive primitive, uint32_t indexCount, uint32_t vertexOffset, uint32_t indexOffset)
			: geometry(geometry)
			, primitive(primitive)
			, vertexCount(indexCount)
			, vertexOffset(vertexOffset)
			, indexOffset(indexOffset)
		{
			RBXASSERT(geometry && isCountValid(primitive, vertexCount));
		}

	}
}
