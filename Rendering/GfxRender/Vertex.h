#pragma once

#include "Util/G3DCore.h"

namespace RBX {

	namespace Graphics {

		/* Position, UV, Color || 36 bytes */
		struct BasicVertex {
			BasicVertex(const RBX::Vector3& Position, const RBX::Vector2& UV, const RBX::Color4& Color)
				: Position(Position)
				, Color(Color)
				, UV(UV)
			{
			}

			BasicVertex()
				: Position(Vector3::zero())
				, UV(Vector2::zero())
				, Color(Color4::zero())
			{
			}

			RBX::Vector3 Position;
			RBX::Vector2 UV;
			RBX::Color4 Color;
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
				, Color(Color4::zero())
				, Normal(Vector3::zero())
				, Tangent(Vector3::zero())
			{
			}

			RBX::Vector3 Position;
			RBX::Vector2 UV;
			RBX::Color4 Color;
			RBX::Vector3 Normal;
			RBX::Vector3 Tangent;
		};

		/* Position, UV, Color, InstanceID, MaterialID || 44 bytes */
		struct InstancedBasicVertex {
			InstancedBasicVertex(const RBX::Vector3& Position, const RBX::Vector2& UV, const RBX::Color4& Color)
				: Position(Position)
				, Color(Color)
				, UV(UV)
			{
			}

			InstancedBasicVertex()
				: Position(Vector3::zero())
				, UV(Vector2::zero())
				, Color(Color4::zero())
				, InstanceID(0u)
				, MaterialID(0u)
			{
			}

			RBX::Vector3 Position;
			RBX::Vector2 UV;
			RBX::Color4 Color;
			unsigned int InstanceID;
			unsigned int MaterialID;
		};

		/* Position, UV, Color, Normal, InstanceID, MaterialID || 56 bytes */
		struct InstancedBasicMaterialVertex {
			InstancedBasicMaterialVertex(const RBX::Vector3& Position, const RBX::Vector2& UV, const RBX::Color4& Color, const RBX::Vector3& Normal)
				: Position(Position)
				, UV(UV)
				, Color(Color)
				, Normal(Normal)
			{
			}

			InstancedBasicMaterialVertex()
				: Position(Vector3::zero())
				, UV(Vector2::zero())
				, Color(Color4::zero())
				, Normal(Vector3::zero())
				, InstanceID(0u)
				, MaterialID(0u)
			{
			}

			RBX::Vector3 Position;
			RBX::Vector2 UV;
			RBX::Color4 Color;
			RBX::Vector3 Normal;
			unsigned int InstanceID;
			unsigned int MaterialID;
		};

		/* Position, UV, Color, Normal, Tangent, InstanceID, MaterialID || 68 bytes */
		struct InstancedMaterialVertex {
			InstancedMaterialVertex(const RBX::Vector3& Position, const RBX::Vector2& UV, const RBX::Color4& Color, const RBX::Vector3& Normal, const RBX::Vector3& Tangent)
				: Position(Position)
				, UV(UV)
				, Color(Color)
				, Normal(Normal)
				, Tangent(Tangent)
			{
			}

			InstancedMaterialVertex()
				: Position(Vector3::zero())
				, UV(Vector2::zero())
				, Color(Color4::zero())
				, Normal(Vector3::zero())
				, Tangent(Vector3::zero())
				, InstanceID(0u)
				, MaterialID(0u)
			{
			}

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