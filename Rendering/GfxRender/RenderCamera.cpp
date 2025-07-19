#include "stdafx.h"
#include "RenderCamera.h"

namespace RBX
{
	namespace Graphics
	{

		static Matrix4 projectionOrtho(float width, float height, float znear, float zfar, bool halfPixelOffset = false)
		{
			// Note: this maps to [0..1] Z range
			float q = 1.0f / (zfar - znear);
			float qn = -znear * q;

			float wOffset = 0.0f;
			float hOffset = 0.0f;

			if (halfPixelOffset)
			{
				wOffset = 1.0f / width;
				hOffset = 1.0f / height;
			}

			return
				Matrix4(
					2.0f / width, 0.0f, 0.0f, -1.0f - wOffset,
					0.0f, 2.0f / height, 0.0f, -1.0f + hOffset,
					0.0f, 0.0f, q, qn,
					0.0f, 0.0f, 0.0f, 1.0f);
		}

		static Matrix4 projectionPerspective(float fovY, float aspect, float znear, float zfar)
		{
			float h = 1.0f / tanf(fovY / 2.0f);
			float w = h / aspect;

			// Note: this maps to [0..1] Z range
			float q = -zfar / (zfar - znear);
			float qn = znear * q;

			return
				Matrix4(
					w, 0.0f, 0.0f, 0.0f,
					0.0f, h, 0.0f, 0.0f,
					0.0f, 0.0f, q, qn,
					0.0f, 0.0f, -1.0f, 0.0f);
		}

		static Matrix4 projectionPerspective(float fovUpTan, float fovDownTan, float fovLeftTan, float fovRightTan, float znear, float zfar)
		{
			float sx = 2.0f / (fovLeftTan + fovRightTan);
			float ox = (fovRightTan - fovLeftTan) / (fovLeftTan + fovRightTan);
			float sy = 2.0f / (fovUpTan + fovDownTan);
			float oy = (fovUpTan - fovDownTan) / (fovUpTan + fovDownTan);

			// Note: this maps to [0..1] Z range
			float q = -zfar / (zfar - znear);
			float qn = znear * q;

			return
				Matrix4(
					sx, 0.0f, ox, 0.0f,
					0.0f, sy, oy, 0.0f,
					0.0f, 0.0f, q, qn,
					0.0f, 0.0f, -1.0f, 0.0f);
		}

		RenderCamera::FrustumPlane::FrustumPlane(const Vector4& plane)
			: plane(plane)
			, planeAbs(G3D::abs(plane.x), G3D::abs(plane.y), G3D::abs(plane.z))
		{
		}

		RenderCamera::RenderCamera()
		{
		}

		void RenderCamera::setViewCFrame(const CoordinateFrame& cframe, float roll)
		{
			position = cframe.translation;
			direction = -cframe.rotation.column(2);
			view = Matrix4::rollDegrees(G3D::toDegrees(roll)) * cframe.inverse().toMatrix4();

			updateViewProjection();
		}

		void RenderCamera::setViewMatrix(const Matrix4& value)
		{
			position = (value.inverse() * Vector4(0.0f, 0.0f, 0.0f, 1.0f)).xyz();
			direction = (value.inverse() * Vector4(0.0f, 0.0f, -1.0f, 0.0f)).xyz();
			view = value;

			updateViewProjection();
		}

		void RenderCamera::setProjectionPerspective(float fovY, float aspect, float znear, float zfar)
		{
			projection = projectionPerspective(fovY, aspect, znear, zfar);

			updateViewProjection();
		}

		void RenderCamera::setProjectionPerspective(float fovUpTan, float fovDownTan, float fovLeftTan, float fovRightTan, float znear, float zfar)
		{
			projection = projectionPerspective(fovUpTan, fovDownTan, fovLeftTan, fovRightTan, znear, zfar);

			updateViewProjection();
		}

		void RenderCamera::setProjectionOrtho(float width, float height, float znear, float zfar, bool halfPixelOffset /*= false*/)
		{
			projection = projectionOrtho(width, height, znear, zfar, halfPixelOffset);

			updateViewProjection();
		}

		void RenderCamera::setProjectionMatrix(const Matrix4& value)
		{
			projection = value;

			updateViewProjection();
		}

		void RenderCamera::changeProjectionPerspectiveZ(float znear, float zfar)
		{
			// Note: this maps to [0..1] Z range
			float q = -zfar / (zfar - znear);
			float qn = znear * q;

			projection[2][2] = q;
			projection[2][3] = qn;
			projection[3][2] = -1.0f;
			projection[3][3] = 0.0f;

			updateViewProjection();
		}

		bool RenderCamera::isVisible(const Extents& extents) const
		{
			Vector4 center = Vector4(extents.center(), 1.0f);
			Vector3 extent = extents.size() * 0.5f;

			for (int i = 0; i < 6; ++i)
			{
				const FrustumPlane& p = frustumPlanes[i];

				float d = p.plane.dot(center);
				float r = p.planeAbs.dot(extent);

				// box is in negative half-space => outside the frustum
				if (d + r < 0.0f)
					return false;
			}

			return true;
		}

		bool RenderCamera::isVisible(const Sphere& sphere) const
		{
			Vector4 center = Vector4(sphere.center, 1.0f);

			float r = sphere.radius;

			for (int i = 0; i < 6; ++i)
			{
				const FrustumPlane& p = frustumPlanes[i];

				float d = p.plane.dot(center);

				// sphere is in negative half-space => outside the frustum
				if (d + r < 0.0f)
					return false;
			}

			return true;
		}

		bool RenderCamera::isVisible(const Extents& extents, const CoordinateFrame& cframe) const
		{
			return isVisible(extents.toWorldSpace(cframe));
		}

		IntersectResult RenderCamera::intersects(const Extents& extents) const
		{
			Vector4 center = Vector4(extents.center(), 1.0f);
			Vector3 extent = extents.size() * 0.5f;

			IntersectResult result = irFull;

			for (int i = 0; i < 6; ++i)
			{
				const FrustumPlane& p = frustumPlanes[i];

				float d = p.plane.dot(center);
				float r = p.planeAbs.dot(extent);

				// box is in negative half-space => outside the frustum
				if (d + r < 0.0f)
					return irNone;

				// box intersects the plane => not fully inside the frustum
				if (d - r < 0.0f)
					result = irPartial;
			}

			return result;
		}

		static Vector4 normalize3(const Vector4& v)
		{
			return v / v.xyz().length();
		}

		void RenderCamera::updateViewProjection()
		{
			viewProjection = projection * view;

			// near: z >= 0
			frustumPlanes[0] = normalize3(viewProjection.row(2));
			// far: z <= w
			frustumPlanes[1] = normalize3(viewProjection.row(3) - viewProjection.row(2));
			// left: x >= -w
			frustumPlanes[2] = normalize3(viewProjection.row(3) + viewProjection.row(0));
			// right: x <= w
			frustumPlanes[3] = normalize3(viewProjection.row(3) - viewProjection.row(0));
			// bottom: y >= -w
			frustumPlanes[4] = normalize3(viewProjection.row(3) + viewProjection.row(1));
			// top: y <= w
			frustumPlanes[5] = normalize3(viewProjection.row(3) - viewProjection.row(1));
		}

	}
}
