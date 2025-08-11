#include "GeometryD3D11.h"

#include "DeviceD3D11.h"
#include "ShaderD3D11.h"

#include "HeadersD3D11.h"

LOGGROUP(Graphics)

namespace RBX {
	namespace Graphics {
		struct BufferUsageD3D11 {
			D3D11_USAGE usage;
			uint32_t cpuAccess;
		};

		static const BufferUsageD3D11 gBufferUsageD3D11[GeometryBuffer::Usage_Count] = {
			{ D3D11_USAGE_DEFAULT, 0u },
			{ D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE },
		};

		static const DXGI_FORMAT gVertexLayoutFormatD3D11[VertexLayout::Format_Count] = {
			DXGI_FORMAT_R32_UINT,
			DXGI_FORMAT_R32_FLOAT,
			DXGI_FORMAT_R32G32_FLOAT,
			DXGI_FORMAT_R32G32B32_FLOAT,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			DXGI_FORMAT_R16G16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R8G8B8A8_UINT,
			DXGI_FORMAT_R8G8B8A8_UNORM,
		};

		static const D3D11_INPUT_CLASSIFICATION gVertexInputD3D11[VertexLayout::Input_Count] = {
			D3D11_INPUT_PER_VERTEX_DATA,
			D3D11_INPUT_PER_INSTANCE_DATA,
		};

		static const LPCSTR gVertexLayoutSemanticD3D11[VertexLayout::Semantic_Count] = {
			"POSITION",
			"TEXCOORD",
			"COLOR",
			"TANGENT",
			"NORMAL",
		};

		static const D3D11_PRIMITIVE_TOPOLOGY gGeometryPrimitiveD3D11[Geometry::Primitive_Count] = {
			D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
			D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
			D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
			D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
		};

		static const D3D11_MAP gLockModeD3D11[GeometryBuffer::Lock_Count] = {
			D3D11_MAP_WRITE,
			D3D11_MAP_WRITE_DISCARD
		};

		VertexLayoutD3D11::VertexLayoutD3D11(Device* device, const std::vector<Element>& elements)
			: VertexLayout(device, elements)
		{
			for (size_t i = 0u; i < elements.size(); ++i) {
				const Element& e = elements[i];
				D3D11_INPUT_ELEMENT_DESC e11 = {};

				e11.SemanticName = gVertexLayoutSemanticD3D11[e.semanticName];
				e11.SemanticIndex = e.semanticIndex;

				e11.Format = gVertexLayoutFormatD3D11[e.format];
				e11.InputSlot = 0u;
				e11.AlignedByteOffset = e.offset;

				e11.InputSlotClass = gVertexInputD3D11[e.inputClass];
				e11.InstanceDataStepRate = e.stepRate;

				elements11.push_back(e11);
			}
		}

		VertexLayoutD3D11::~VertexLayoutD3D11() {
			for (size_t i = 0u; i < shaders.size(); ++i) {
				shared_ptr<VertexShaderD3D11> vertexShader = shaders[i].lock();

				if (vertexShader)
					vertexShader->removeLayout(this);
			}

			static_cast<DeviceD3D11*>(device)->getImmediateContextD3D11()->invalidateCachedVertexLayout();
		}

		void VertexLayoutD3D11::registerShader(const shared_ptr<VertexShaderD3D11>& shader) {
			shaders.push_back(weak_ptr<VertexShaderD3D11>(shader));
		}

		template <typename Base> GeometryBufferD3D11<Base>::GeometryBufferD3D11(Device* device, size_t elementSize, size_t elementCount, GeometryBuffer::Usage usage)
			: Base(device, elementSize, elementCount, usage)
			, locked(nullptr)
			, object(nullptr)
		{
		}

		template <typename Base> void GeometryBufferD3D11<Base>::create(uint32_t bindFlags) {
			ID3D11Device* device11 = static_cast<DeviceD3D11*>(device)->getDevice11();

			D3D11_BUFFER_DESC bd = {};
			bd.Usage = gBufferUsageD3D11[usage].usage;
			bd.ByteWidth = elementSize * elementCount;
			bd.BindFlags = bindFlags;
			bd.CPUAccessFlags = gBufferUsageD3D11[usage].cpuAccess;
			bd.MiscFlags = 0u;
			bd.StructureByteStride = 0u;

			HRESULT hr = device11->CreateBuffer(&bd, nullptr, &object);

			if (FAILED(hr))
				throw RBX::runtime_error("Couldn't create geometry buffer: %x", hr);
		}

		template <typename Base> GeometryBufferD3D11<Base>::~GeometryBufferD3D11() {
			RBXASSERT(!locked);

			ReleaseCheck(object);
		}

		template <typename Base> void* GeometryBufferD3D11<Base>::lock(GeometryBuffer::LockMode mode) {
			RBXASSERT(!locked);

			if (usage == Usage::Usage_Static)
				locked = new char[elementSize * elementCount];
			else {
				ID3D11DeviceContext* context11 = static_cast<DeviceD3D11*>(device)->getImmediateContext11();
				D3D11_MAP mapMode = gLockModeD3D11[mode];

				D3D11_MAPPED_SUBRESOURCE resource;
				HRESULT hr = context11->Map(object, 0u, mapMode, 0u, &resource);

				if (FAILED(hr)) {
					FASTLOG2(FLog::Graphics, "Failed to lock VB (size %d): %x", elementCount * elementSize, hr);
					return nullptr;
				}

				locked = resource.pData;
			}

			RBXASSERT(locked);
			return locked;
		}

		template <typename Base> void GeometryBufferD3D11<Base>::unlock() {
			RBXASSERT(locked);

			if (usage == Usage::Usage_Static) {
				upload(0u, locked, elementCount * elementSize);
				delete[] static_cast<char*>(locked);
			}
			else {
				ID3D11DeviceContext* context11 = static_cast<DeviceD3D11*>(device)->getImmediateContext11();

				context11->Unmap(object, 0u);
			}

			locked = nullptr;
		}

		template <typename Base> void GeometryBufferD3D11<Base>::upload(uint32_t offset, const void* data, size_t size) {
			ID3D11DeviceContext* context11 = static_cast<DeviceD3D11*>(device)->getImmediateContext11();

			D3D11_BOX box = {};
			box.left = offset;
			box.right = offset + size;
			box.top = 0u;
			box.bottom = 1u;
			box.front = 0u;
			box.back = 1u;

			context11->UpdateSubresource(object, 0u, &box, data, 0u, 0u);
		}

		VertexBufferD3D11::VertexBufferD3D11(Device* device, size_t elementSize, size_t elementCount, Usage usage)
			: GeometryBufferD3D11<VertexBuffer>(device, elementSize, elementCount, usage)
		{
			create(D3D11_BIND_VERTEX_BUFFER);
		}

		VertexBufferD3D11::~VertexBufferD3D11()
		{
		}

		IndexBufferD3D11::IndexBufferD3D11(Device* device, size_t elementSize, size_t elementCount, Usage usage)
			: GeometryBufferD3D11<IndexBuffer>(device, elementSize, elementCount, usage)
		{
			if (elementSize != 1u && elementSize != 2u && elementSize != 4u)
				throw RBX::runtime_error("Invalid element size: %d", (int)elementSize);

			create(D3D11_BIND_INDEX_BUFFER);
		}

		IndexBufferD3D11::~IndexBufferD3D11()
		{
		}

		GeometryD3D11::GeometryD3D11(Device* device, const shared_ptr<VertexLayout>& layout, const shared_ptr<VertexBuffer>& vertexBuffer, const shared_ptr<IndexBuffer>& indexBuffer)
			: Geometry(device, layout, vertexBuffer, indexBuffer)
		{
		}

		GeometryD3D11::~GeometryD3D11() {
			static_cast<DeviceD3D11*>(device)->getImmediateContextD3D11()->invalidateCachedGeometry();
		}

		void GeometryD3D11::draw(Geometry::Primitive primitive, uint32_t vertexCount, uint32_t vertexOffset, uint32_t indexOffset, VertexLayoutD3D11** layoutCache, GeometryD3D11** geometryCache, ShaderProgramD3D11** programCache) {
			ID3D11DeviceContext* context11 = static_cast<DeviceD3D11*>(device)->getImmediateContext11();

			context11->IASetPrimitiveTopology(gGeometryPrimitiveD3D11[primitive]);

			if (*layoutCache != layout.get()) {
				auto* vertexLayout = static_cast<VertexLayoutD3D11*>(layout.get());
				*layoutCache = vertexLayout;

				context11->IASetInputLayout((*programCache)->getInputLayout11(vertexLayout));
			}

			if (*geometryCache != this) {
				*geometryCache = this;

				/* Vertex Buffer */
				{
					auto* vb = static_cast<VertexBufferD3D11*>(vertexBuffer.get());
					ID3D11Buffer* vb11 = vb->getObject();
					size_t offsetVB = 0u;
					size_t stride = vb->getElementSize();

					context11->IASetVertexBuffers(0u, 1u, &vb11, &stride, &offsetVB);
				}

				/* Index Buffer */
				if (indexBuffer) {
					DXGI_FORMAT format;

					switch (indexBuffer->getElementSize()) {
					case (1u):
						format = DXGI_FORMAT_R8_UINT;
					case (2u):
						format = DXGI_FORMAT_R16_UINT;
					default:
						format = DXGI_FORMAT_R32_UINT;
					}

					context11->IASetIndexBuffer(static_cast<IndexBufferD3D11*>(indexBuffer.get())->getObject(), format, 0u);
				}
			}

			if (indexBuffer)
				context11->DrawIndexed(vertexCount, indexOffset, vertexOffset);
			else
				context11->Draw(vertexCount, vertexOffset);
		}

		void GeometryD3D11::drawInstanced(Geometry::Primitive primitive, uint32_t instanceCount, uint32_t vertexCount, uint32_t vertexOffset, uint32_t indexOffset, VertexLayoutD3D11** layoutCache, GeometryD3D11** geometryCache, ShaderProgramD3D11** programCache) {
			ID3D11DeviceContext* context11 = static_cast<DeviceD3D11*>(device)->getImmediateContext11();

			context11->IASetPrimitiveTopology(gGeometryPrimitiveD3D11[primitive]);

			if (*layoutCache != layout.get()) {
				auto* vertexLayout = static_cast<VertexLayoutD3D11*>(layout.get());
				*layoutCache = vertexLayout;

				context11->IASetInputLayout((*programCache)->getInputLayout11(vertexLayout));
			}

			if (*geometryCache != this) {
				*geometryCache = this;

				/* Vertex Buffer */
				{
					auto* vb = static_cast<VertexBufferD3D11*>(vertexBuffer.get());
					ID3D11Buffer* vb11 = vb->getObject();
					size_t offsetVB = 0u;
					size_t stride = vb->getElementSize();

					context11->IASetVertexBuffers(0u, 1u, &vb11, &stride, &offsetVB);
				}

				/* Index Buffer */
				if (indexBuffer) {
					DXGI_FORMAT format;

					switch (indexBuffer->getElementSize()) {
					case (1u):
						format = DXGI_FORMAT_R8_UINT;
					case (2u):
						format = DXGI_FORMAT_R16_UINT;
					default:
						format = DXGI_FORMAT_R32_UINT;
					}

					context11->IASetIndexBuffer(static_cast<IndexBufferD3D11*>(indexBuffer.get())->getObject(), format, 0u);
				}
			}

			if (indexBuffer)
				context11->DrawIndexedInstanced(vertexCount, instanceCount, indexOffset, vertexOffset, 0u);
			else
				context11->DrawInstanced(vertexCount, instanceCount, vertexOffset, 0u);
		}

	}
}
