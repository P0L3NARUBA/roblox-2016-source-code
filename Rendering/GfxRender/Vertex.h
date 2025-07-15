#pragma once

#include "Util/G3DCore.h"

namespace RBX {

	namespace Graphics {

		struct Vertex
		{
			Vertex(const RBX::Vector3& Position, const RBX::Vector2& UV, const RBX::Color4& Color)
				: Position(Position)
				, Color(Color)
				, UV(UV)
			{
			}

			Vertex(const RBX::Vector3& Position, const RBX::Vector2& UV, const RBX::Color4& Color, const RBX::Vector3& Normal)
				: Position(Position)
				, UV(UV)
				, Color(Color)
				, Normal(Normal)
			{
			}

			Vertex(const RBX::Vector3& Position, const RBX::Vector2& UV, const RBX::Color4& Color, const RBX::Vector3& Normal, const RBX::Vector3& Tangent)
				: Position(Position)
				, UV(UV)
				, Color(Color)
				, Normal(Normal)
				, Tangent(Tangent)
			{
			}

			Vertex() {}

			RBX::Vector3 Position;
			RBX::Vector2 UV;
			RBX::Color4 Color;
			RBX::Vector3 Normal;
			RBX::Vector3 Tangent;
			unsigned int InstanceID;
			unsigned int MaterialID;
		};
	}
}