#pragma once

#include "Util/G3DCore.h"

#include "GfxCore/Geometry.h"

namespace RBX
{
	struct RenderPassStats;
}

namespace RBX
{

	enum BatchTextureType
	{
		BatchTextureType_Color,
		BatchTextureType_Font,
		BatchTextureType_Count
	};

	namespace Graphics
	{
		class VisualEngine;
		class DeviceContext;
		class Texture;
		class RenderCamera;

		struct Vertex
		{
			Vertex(const RBX::Vector4& Position, const RBX::Vector2& UV, const RBX::Color4& Color)
				: Position(Position)
				, Color(Color)
				, UV(UV)
			{
			}

			Vertex(const RBX::Vector4& Position, const RBX::Vector2& UV, const RBX::Color4& Color, const RBX::Vector3& Normal)
				: Position(Position)
				, UV(UV)
				, Color(Color)
				, Normal(Normal)
			{
			}

			Vertex(const RBX::Vector4& Position, const RBX::Vector2& UV, const RBX::Color4& Color, const RBX::Vector3& Tangent, const RBX::Vector3& Bitangent, const RBX::Vector3& Normal)
				: Position(Position)
				, UV(UV)
				, Color(Color)
				, Tangent(Tangent)
				, Bitangent(Bitangent)
				, Normal(Normal)
			{
			}

			Vertex() {}

			RBX::Vector4 Position;
			RBX::Vector2 UV;
			RBX::Color4 Color;
			RBX::Vector3 Tangent;
			RBX::Vector3 Bitangent;
			RBX::Vector3 Normal;
			unsigned int InstanceID;
			unsigned int MaterialID;
		};

		class VertexStreamer
		{
		public:
			VertexStreamer(VisualEngine* visualEngine, bool hasNormal, bool hasAdvancedNormal, bool isInstanced);
			~VertexStreamer();

			void renderPrepare();
			void render3D(DeviceContext* context, const RenderCamera& camera, RenderPassStats& stats);
			void render3DNoDepth(DeviceContext* context, const RenderCamera& camera, RenderPassStats& stats, int renderIndex);
			void render2D(DeviceContext* context, unsigned int viewWidth, unsigned int viewHeight, RenderPassStats& stats);
			void render2DVR(DeviceContext* context, unsigned int viewWidth, unsigned int viewHeight, RenderPassStats& stats);
			void renderFinish();

			void cleanUpFrameData();

			void rectBlt(const shared_ptr<Texture>& texptr, const Color4& color4, const Vector2& x0y0, const Vector2& x1y0, const Vector2& x0y1, const Vector2& x1y1, const Vector2& tex0, const Vector2& tex1, BatchTextureType batchTexType, bool ignoreTexture = false);

			void line(float x1, float y1, float x2, float y2, const Color4& color4);
			void line3d(float x1, float y1, float z1, float x2, float y2, float z2, const Color4& color4);

			void spriteBlt3D(const shared_ptr<Texture>& texptr, const Color4& color4, BatchTextureType batchTexType, const Vector3& x0y0, const Vector3& x1y0, const Vector3& x0y1, const Vector3& x1y1, const Vector2& tex0, const Vector2& tex1, int zIndex, bool alwaysOnTop);

			void triangleList2d(const Color4& color4, const Vector2* v, int vcount, const short* i, int icount);
			void triangleList(const Color4& color4, const CoordinateFrame& cframe, const Vector3* v, int vcount, const short* i, int icount);

		private:
			enum CoordinateSpace
			{
				CS_WorldSpace = 0,
				CS_ScreenSpace = 1,
				CS_WorldSpaceNoDepth = 2,
				CS_NumTypes
			};

			struct VertexChunk
			{
				VertexChunk(const shared_ptr<Texture>& texture, Geometry::Primitive primitive, unsigned int vertexStart, unsigned int vertexCount, BatchTextureType batchTexType, int zIndex, bool forceTop)
					: texture(texture)
					, primitive(primitive)
					, vertexStart(vertexStart)
					, vertexCount(vertexCount)
					, batchTextureType(batchTexType)
					, index(zIndex)
					, alwaysOnTop(forceTop)
				{
				}

				VertexChunk()
				{
				}

				shared_ptr<Texture> texture;
				Geometry::Primitive primitive;

				unsigned int vertexStart;
				unsigned int vertexCount;

				int index;
				bool alwaysOnTop;

				BatchTextureType batchTextureType;
			};

			VisualEngine* visualEngine;

			bool colorOrderBGR;
			bool halfPixelOffset;

			G3D::Array<VertexChunk> chunks[CS_NumTypes];

			G3D::Array<Vertex> vertexData;
			G3D::Array<VertexChunk*> worldSpaceZDepthLayers[Adorn::maximumZIndex + 1];
			shared_ptr<VertexBuffer> vertexBuffer;
			shared_ptr<VertexLayout> vertexLayout;
			shared_ptr<Geometry> geometry;

			VertexChunk* prepareChunk(const shared_ptr<Texture>& texPtr, Geometry::Primitive primitive, unsigned int vertexCount, CoordinateSpace coordinateSpace, BatchTextureType batchTextureType, bool ignoreTexture = false, int zIndex = -1, bool forceTop = false);
			void renderInternal(DeviceContext* context, CoordinateSpace coordinateSpace, const RenderCamera& camera, RenderPassStats& stats, int renderIndex = -1);
		};
	}
}