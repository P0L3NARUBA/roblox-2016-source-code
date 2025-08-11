#pragma once

#include "Util/G3DCore.h"
#include "GfxCore/Geometry.h"

namespace RBX {

	namespace Graphics {

		/* Position || 12 bytes */
		struct BasicVertex {
			BasicVertex(const RBX::Vector3& Position)
				: Position(Position)
			{
			}

			BasicVertex()
				: Position(Vector3::zero())
			{
			}

			static inline std::vector<VertexLayout::Element> getVertexLayout() {
				std::vector<VertexLayout::Element> elements;
				elements.push_back(VertexLayout::Element(VertexLayout::Semantic_Position, 0u, VertexLayout::Format_Float3, 0u, VertexLayout::Input_Vertex));

				return elements;
			}

			RBX::Vector3 Position;
		};

		/* Position, UV, Color, Normal || 48 bytes */
		struct BasicMaterialVertex {
			BasicMaterialVertex(const RBX::Vector3& Position, const RBX::Vector2& UV, const RBX::Color4& Color, const RBX::Vector3& Normal)
				: Position(Position)
				, UV(UV)
				, Color(Color)
				, Normal(Normal)
			{
			}

			BasicMaterialVertex()
				: Position(Vector3::zero())
				, UV(Vector2::zero())
				, Color(Color4::zero())
				, Normal(Vector3::zero())
			{
			}

			static inline std::vector<VertexLayout::Element> getVertexLayout() {
				std::vector<VertexLayout::Element> elements;
				elements.push_back(VertexLayout::Element(VertexLayout::Semantic_Position, 0u, VertexLayout::Format_Float3, 0u, VertexLayout::Input_Vertex));
				elements.push_back(VertexLayout::Element(VertexLayout::Semantic_Texture, 0u, VertexLayout::Format_Float2, 12u, VertexLayout::Input_Vertex));
				elements.push_back(VertexLayout::Element(VertexLayout::Semantic_Color, 0u, VertexLayout::Format_Float4, 20u, VertexLayout::Input_Vertex));
				elements.push_back(VertexLayout::Element(VertexLayout::Semantic_Normal, 0u, VertexLayout::Format_Float3, 36u, VertexLayout::Input_Vertex));

				return elements;
			}

			RBX::Vector3 Position;
			RBX::Vector2 UV;
			RBX::Color4 Color;
			RBX::Vector3 Normal;
		};

		/* Position, UV, Color, Normal, Tangent || 60 bytes */
		struct MaterialVertex {
			MaterialVertex(const RBX::Vector3& Position, const RBX::Vector2& UV, const RBX::Color4& Color, const RBX::Vector3& Normal, const RBX::Vector3& Tangent)
				: Position(Position)
				, UV(UV)
				, Color(Color)
				, Normal(Normal)
				, Tangent(Tangent)
			{
			}

			MaterialVertex()
				: Position(Vector3::zero())
				, UV(Vector2::zero())
				, Color(Color4::one())
				, Normal(Vector3::zero())
				, Tangent(Vector3::normal())
			{
			}

			static inline std::vector<VertexLayout::Element> getVertexLayout() {
				std::vector<VertexLayout::Element> elements;
				elements.push_back(VertexLayout::Element(VertexLayout::Semantic_Position, 0u, VertexLayout::Format_Float3, 0u, VertexLayout::Input_Vertex));
				elements.push_back(VertexLayout::Element(VertexLayout::Semantic_Texture, 0u, VertexLayout::Format_Float2, 12u, VertexLayout::Input_Vertex));
				elements.push_back(VertexLayout::Element(VertexLayout::Semantic_Color, 0u, VertexLayout::Format_Float4, 20u, VertexLayout::Input_Vertex));
				elements.push_back(VertexLayout::Element(VertexLayout::Semantic_Normal, 0u, VertexLayout::Format_Float3, 36u, VertexLayout::Input_Vertex));
				elements.push_back(VertexLayout::Element(VertexLayout::Semantic_Tangent, 0u, VertexLayout::Format_Float3, 48u, VertexLayout::Input_Vertex));

				return elements;
			}

			RBX::Vector3 Position;
			RBX::Vector2 UV;
			RBX::Color4 Color;
			RBX::Vector3 Normal;
			RBX::Vector3 Tangent;
		};
	}
}