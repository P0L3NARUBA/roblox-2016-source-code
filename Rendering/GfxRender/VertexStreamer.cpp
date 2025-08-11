#include "stdafx.h"
#include "VertexStreamer.h"

#include "VisualEngine.h"
#include "SceneManager.h"
#include "ShaderManager.h"
#include "TextureManager.h"

#include "Util.h"

#include "GfxCore/Device.h"
#include "GfxCore/States.h"

#include "GfxBase/RenderStats.h"

#include "rbx/Profiler.h"

FASTFLAG(GUIZFighterGPU)
FASTFLAG(UseDynamicTypesetterUTF8)

namespace RBX {
	namespace Graphics {

		static const uint32_t kVertexBufferMaxCount = 512u * 1024u;

		VertexStreamer::VertexStreamer(VisualEngine* visualEngine)
			: visualEngine(visualEngine)
		{
			std::vector<VertexLayout::Element> elements;
			/*elements.push_back(VertexLayout::Element(0u, 0u, VertexLayout::Format_Float3, VertexLayout::Input_Vertex, VertexLayout::Semantic_Position));
			elements.push_back(VertexLayout::Element(0u, 12u, VertexLayout::Format_Float2, VertexLayout::Input_Vertex, VertexLayout::Semantic_Texture));
			elements.push_back(VertexLayout::Element(0u, 20u, VertexLayout::Format_Float4, VertexLayout::Input_Vertex, VertexLayout::Semantic_Color));*/

			vertexLayout = visualEngine->getDevice()->createVertexLayout(elements);

			const DeviceCaps& caps = visualEngine->getDevice()->getCaps();

			colorOrderBGR = caps.colorOrderBGR;
			halfPixelOffset = caps.needsHalfPixelOffset;
		}

		VertexStreamer::~VertexStreamer()
		{
		}

		void VertexStreamer::cleanUpFrameData() {
			for (size_t cs = 0u; cs < CS_NumTypes; ++cs) {
				chunks[cs].resize(0u);
			}

			for (size_t i = 0u; i < 10u; ++i) {
				worldSpaceZDepthLayers[i].resize(0u);
			}

			vertexData.fastClear();
		}

		void VertexStreamer::renderPrepare() {
			RBXPROFILER_SCOPE("Render", "prepare");

			RBXASSERT(vertexData.size() <= kVertexBufferMaxCount);

			// allocate hardware buffer according to total # of vertices
			size_t bufferSize = 32u;

			while (bufferSize < vertexData.size())
				bufferSize += bufferSize / 2u;

			bufferSize = std::min(bufferSize, kVertexBufferMaxCount);

			if (!vertexBuffer || vertexBuffer->getElementCount() < bufferSize) {
				vertexBuffer = visualEngine->getDevice()->createVertexBuffer(sizeof(BasicVertex), bufferSize, GeometryBuffer::Usage_Dynamic);

				geometry = visualEngine->getDevice()->createGeometry(vertexLayout, vertexBuffer, shared_ptr<IndexBuffer>());
			}

			if (vertexData.size() > 0u) {
				void* bufferData = vertexBuffer->lock(GeometryBuffer::Lock_Discard);

				memcpy(bufferData, &vertexData[0], sizeof(BasicVertex) * vertexData.size());

				vertexBuffer->unlock();
			}

			if (chunks[CS_WorldSpaceNoDepth].size() > 0u) {
				for (G3D::Array<VertexChunk>::iterator iter = chunks[CS_WorldSpaceNoDepth].begin(); iter != chunks[CS_WorldSpaceNoDepth].end(); ++iter) {
					RBXASSERT(iter->index >= 0u);

					worldSpaceZDepthLayers[iter->index].push_back(&(*iter));
				}
			}
		}

		void VertexStreamer::render3D(DeviceContext* context, const RenderCamera& camera, RenderPassStats& stats) {
			RBXPROFILER_SCOPE("Render", "render3D");

			PIX_SCOPE(context, "3D");
			renderInternal(context, CS_WorldSpace, camera, stats);
		}

		void VertexStreamer::render3DNoDepth(DeviceContext* context, const RenderCamera& camera, RenderPassStats& stats, int32_t renderIndex) {
			RBXPROFILER_SCOPE("Render", "render3DNoDepth");

			PIX_SCOPE(context, "3D");
			renderInternal(context, CS_WorldSpaceNoDepth, camera, stats, renderIndex);
		}

		void VertexStreamer::render2D(DeviceContext* context, uint32_t viewWidth, uint32_t viewHeight, RenderPassStats& stats) {
			RBXPROFILER_SCOPE("Render", "render2D");

			RenderCamera orthoCamera;
			orthoCamera.setViewMatrix(Matrix4::identity());
			orthoCamera.setProjectionOrtho(viewWidth, viewHeight, -1.0f, 1.0f, halfPixelOffset);

			renderInternal(context, CS_ScreenSpace, orthoCamera, stats);
		}

		void VertexStreamer::render2DVR(DeviceContext* context, uint32_t viewWidth, uint32_t viewHeight, RenderPassStats& stats) {
			RBXPROFILER_SCOPE("Render", "render2DVR");

			float scaleX = float(viewWidth) / float(visualEngine->getViewWidth());
			float scaleY = float(viewHeight) / float(visualEngine->getViewHeight());

			RenderCamera orthoCamera;
			orthoCamera.setViewMatrix(Matrix4::identity());
			orthoCamera.setProjectionOrtho(viewWidth / scaleX, viewHeight / scaleY, -1.0f, 1.0f, halfPixelOffset);

			orthoCamera.setProjectionMatrix(Matrix4::scale(0.5f, 0.5f, 1.0f) * orthoCamera.getProjectionMatrix());

			renderInternal(context, CS_ScreenSpace, orthoCamera, stats);
		}

		void VertexStreamer::renderFinish() {
			cleanUpFrameData();
		}

		void VertexStreamer::renderInternal(DeviceContext* context, CoordinateSpace coordinateSpace, const RenderCamera& camera, RenderPassStats& stats, int32_t renderIndex) {
			if (chunks[coordinateSpace].size() == 0u || (coordinateSpace == CS_WorldSpaceNoDepth && worldSpaceZDepthLayers[renderIndex].size() == 0u))
				return;

			// Get resources
			shared_ptr<Texture> defaultTexture = visualEngine->getTextureManager()->getFallbackTexture(TextureManager::Fallback_White);
			RBXASSERT(defaultTexture);

			static const SamplerState defaultSamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp);
			static const SamplerState wrapSamplerState(SamplerState::Filter_Linear, SamplerState::Address_Wrap);

			std::string vsName = (coordinateSpace == CS_WorldSpace) ? "UIFogVS" : "UIVS";
			std::string fsName = (coordinateSpace == CS_WorldSpace) ? "UIFogFS" : "UIFS";

			shared_ptr<ShaderProgram> shaderProgram = visualEngine->getShaderManager()->getProgram(vsName, fsName);

			if (!shaderProgram)
				return;

			GlobalShaderData shaderData = visualEngine->getSceneManager()->readGlobalShaderData();

			shaderData.setCamera(camera);

			context->updateGlobalConstantData(&shaderData, sizeof(GlobalShaderData));

			context->bindProgram(shaderProgram.get());

			// init luminance sampling
			static const float zFighterOffset = FFlag::GUIZFighterGPU ? 0.001f : 0.0f;
			static const Vector4 luminanceSamplingOff = Vector4(0.0f, 0.0f, 0.0f, zFighterOffset);
			static const Vector4 luminanceSamplingOn = Vector4(1.0f, 1.0f, 1.0f, zFighterOffset);
			//int uiParamsHandle = shaderProgram->getConstantHandle("UIParams");
			BatchTextureType currentTextureType = BatchTextureType_Count; // invalid so it will be set at first iteration

			context->setRasterizerState(RasterizerState::Cull_Back);
			context->setBlendState(BlendState(BlendState::Mode_AlphaBlend));
			context->setDepthState(DepthState(coordinateSpace == CS_WorldSpace ? DepthState::Function_GreaterEqual : DepthState::Function_Always, false));

			// Only required for FFP
			//context->setWorldTransforms4x3(NULL, 0);

			stats.passChanges++;

			Texture* currentTexture = nullptr;

			size_t arraySize = coordinateSpace == CS_WorldSpaceNoDepth ? worldSpaceZDepthLayers[renderIndex].size() : chunks[coordinateSpace].size();

			for (size_t i = 0u; i < arraySize; ++i) {
				const VertexChunk& currChunk = coordinateSpace == CS_WorldSpaceNoDepth ? *worldSpaceZDepthLayers[renderIndex][i] : chunks[coordinateSpace][i];

				if (currChunk.index >= 0)
					context->setDepthState(DepthState(currChunk.alwaysOnTop ? DepthState::Function_Always : DepthState::Function_GreaterEqual, false));

				Texture* texture = currChunk.texture.get();

				// Change texture
				if (texture != currentTexture || i == 0u) {
					currentTexture = texture;

					if (currentTextureType != currChunk.batchTextureType) {
						/*if (currChunk.batchTextureType == BatchTextureType_Font)
							context->setConstant(uiParamsHandle, &luminanceSamplingOn[0], 1);
						else
							context->setConstant(uiParamsHandle, &luminanceSamplingOff[0], 1);*/

						currentTextureType = currChunk.batchTextureType;
					}

					if (currChunk.batchTextureType == BatchTextureType_Font && FFlag::UseDynamicTypesetterUTF8)
						context->bindTexture(0u, texture ? texture : defaultTexture.get(), wrapSamplerState);
					else
						context->bindTexture(0u, texture ? texture : defaultTexture.get(), defaultSamplerState);
				}

				// Render!
				context->draw(geometry.get(), currChunk.primitive, currChunk.vertexStart, currChunk.vertexCount, 0u);

				stats.batches++;
				stats.faces += currChunk.vertexCount / 3u;
				stats.vertices += currChunk.vertexCount;
			}
		}

		VertexStreamer::VertexChunk* VertexStreamer::prepareChunk(const shared_ptr<Texture>& texture, Geometry::Primitive primitive, uint32_t vertexCount, CoordinateSpace coordinateSpace, BatchTextureType batchTexType, bool ignoreTexture, int32_t zIndex, bool forceTop) {
			uint32_t vertexStart = vertexData.size();

			if (vertexStart + vertexCount > kVertexBufferMaxCount)
				return nullptr;

			const shared_ptr<Texture>& actualTexture = ignoreTexture ? visualEngine->getTextureManager()->getFallbackTexture(TextureManager::Fallback_White) : texture;
			G3D::Array<VertexChunk>& orderedChunks = chunks[coordinateSpace];

			bool newsection = true;

			if (orderedChunks.size() > 0u) {
				VertexChunk& last = orderedChunks.back();

				newsection =
					(last.texture != actualTexture) ||
					last.primitive != primitive ||
					last.vertexStart + last.vertexCount != vertexStart ||
					last.index != zIndex ||
					last.alwaysOnTop != forceTop;
			}

			if (newsection) {
				orderedChunks.push_back(VertexChunk(actualTexture, primitive, vertexStart, vertexCount, batchTexType, zIndex, forceTop));
			}
			else {
				RBXASSERT(orderedChunks.back().batchTextureType == batchTexType);
				orderedChunks.back().vertexCount += vertexCount;
			}

			return &orderedChunks.back();
		}

		void VertexStreamer::rectBlt(
			const shared_ptr<Texture>& texptr, const Color4& color4,
			const Vector2& x0y0, const Vector2& x1y0, const Vector2& x0y1, const Vector2& x1y1,
			const Vector2& tex0, const Vector2& tex1, BatchTextureType batchTexType, bool ignoreTexture)
		{
			if (prepareChunk(texptr, Geometry::Primitive_Triangles, 6u, CS_ScreenSpace, batchTexType, ignoreTexture)) {
				float z = 0.0f;
				
				/*vertexData.push_back(BasicVertex(Vector3(x0y1.x, x0y1.y, z), Vector2(tex0.x, tex1.y), color4));
				vertexData.push_back(BasicVertex(Vector3(x1y0.x, x1y0.y, z), Vector2(tex1.x, tex0.y), color4));
				vertexData.push_back(BasicVertex(Vector3(x0y0.x, x0y0.y, z), Vector2(tex0.x, tex0.y), color4));
				vertexData.push_back(BasicVertex(Vector3(x0y1.x, x0y1.y, z), Vector2(tex0.x, tex1.y), color4));
				vertexData.push_back(BasicVertex(Vector3(x1y1.x, x1y1.y, z), Vector2(tex1.x, tex1.y), color4));
				vertexData.push_back(BasicVertex(Vector3(x1y0.x, x1y0.y, z), Vector2(tex1.x, tex0.y), color4));*/
			}
		}

		void VertexStreamer::spriteBlt3D(const shared_ptr<Texture>& texptr, const Color4& color4, BatchTextureType batchTexType,
			const Vector3& x0y0, const Vector3& x1y0, const Vector3& x0y1, const Vector3& x1y1,
			const Vector2& tex0, const Vector2& tex1, int32_t zIndex, bool alwaysOnTop)
		{
			if (prepareChunk(texptr, Geometry::Primitive_Triangles, 6u, alwaysOnTop || zIndex >= 0 ? CS_WorldSpaceNoDepth : CS_WorldSpace, batchTexType, false, zIndex, alwaysOnTop)) {
				/*vertexData.push_back(BasicVertex(Vector3(x0y1), Vector2(tex0.x, tex1.y), color4));
				vertexData.push_back(BasicVertex(Vector3(x1y0), Vector2(tex1.x, tex0.y), color4));
				vertexData.push_back(BasicVertex(Vector3(x0y0), Vector2(tex0.x, tex0.y), color4));
				vertexData.push_back(BasicVertex(Vector3(x0y1), Vector2(tex0.x, tex1.y), color4));
				vertexData.push_back(BasicVertex(Vector3(x1y1), Vector2(tex1.x, tex1.y), color4));
				vertexData.push_back(BasicVertex(Vector3(x1y0), Vector2(tex1.x, tex0.y), color4));*/
			}
		}

		void VertexStreamer::triangleList2d(const Color4& color4, const Vector2* v, uint32_t vcount, const short* indices, uint32_t icount) {
			if (prepareChunk(shared_ptr<Texture>(), Geometry::Primitive_Triangles, icount, CS_ScreenSpace, BatchTextureType_Color)) {
				RBXASSERT(icount % 3u == 0u);
				//todo: we currently just ignore the nice indexing work done. todo: support indexing.
				for (size_t i = 0u; i < icount; ++i) {
					//vertexData.push_back(BasicVertex(Vector3(v[indices[i]], 0.0f), Vector2(), color4));
				}
			}
		}

		void VertexStreamer::triangleList(const Color4& color4, const CoordinateFrame& cframe, const Vector3* v, uint32_t vcount, const short* indices, uint32_t icount) {
			if (prepareChunk(shared_ptr<Texture>(), Geometry::Primitive_Triangles, icount, CS_WorldSpace, BatchTextureType_Color)) {
				RBXASSERT(icount % 3u == 0u);
				//todo: we currently just ignore the nice indexing work done. todo: support indexing.
				for (size_t i = 0u; i < icount; ++i) {
					//vertexData.push_back(BasicVertex(Vector3(cframe.pointToWorldSpace(v[indices[i]])), Vector2(), color4));
				}
			}
		}

		void VertexStreamer::line(float x1, float y1, float x2, float y2, const Color4& color4) {
			if (prepareChunk(shared_ptr<Texture>(), Geometry::Primitive_Lines, 2, CS_ScreenSpace, BatchTextureType_Color)) {
				float z = 0.0f;
				
				/*vertexData.push_back(BasicVertex(Vector3(x1, y1, z), Vector2(), color4));
				vertexData.push_back(BasicVertex(Vector3(x2, y2, z), Vector2(), color4));*/
			}
		}

		void VertexStreamer::line3d(float x1, float y1, float z1, float x2, float y2, float z2, const Color4& color4) {
			if (prepareChunk(shared_ptr<Texture>(), Geometry::Primitive_Lines, 2, CS_WorldSpace, BatchTextureType_Color)) {
				/*vertexData.push_back(BasicVertex(Vector3(x1, y1, z1), Vector2(), color4));
				vertexData.push_back(BasicVertex(Vector3(x2, y2, z2), Vector2(), color4));*/
			}
		}

	}
}
