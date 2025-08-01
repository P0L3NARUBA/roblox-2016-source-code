#include "stdafx.h"

#include "V8World/Ball.h"
#include "G3D/CollisionDetection.h"
#include "G3D/Sphere.h"
#include "Util/Units.h"
#include "Util/Math.h"


namespace RBX {

	Matrix3 Ball::getMomentSolid(float mass) const {
		float c = mass * (2.0f / 5.0f) * realRadius * realRadius;

		return Math::fromDiagonal(Vector3(c, c, c));
	}


	float Ball::getVolume() const {
		// sphere volume == 4/3 * pi * r^3
		return 1.33333333f * Math::pif() * realRadius * realRadius * realRadius;
	}


	bool Ball::hitTest(const RbxRay& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal) {
		bool hit = (G3D::CollisionDetection::collisionTimeForMovingPointFixedSphere(
			rayInMe.origin(),
			rayInMe.direction(),
			G3D::Sphere(Vector3::zero(), realRadius),
			localHitPoint,
			surfaceNormal)

			!= G3D::inf());

		return hit;
	}


	void Ball::setSize(const G3D::Vector3& _size) {
		Super::setSize(_size);
		RBXASSERT(getSize() == _size);

		realRadius = _size.x * 0.5f;

		if (bulletCollisionObject)
			updateBulletCollisionData();
	}

	size_t Ball::closestSurfaceToPoint(const Vector3& pointInBody)  const {
		float maxDotProd = pointInBody.x;
		size_t id = 0u;

		if (pointInBody.y > maxDotProd) {
			maxDotProd = pointInBody.y;
			id = 1u;
		}

		if (pointInBody.z > maxDotProd) {
			maxDotProd = pointInBody.z;
			id = 2u;
		}

		if (-pointInBody.x > maxDotProd) {
			maxDotProd = -pointInBody.x;
			id = 3u;
		}

		if (-pointInBody.y > maxDotProd) {
			maxDotProd = -pointInBody.y;
			id = 4u;
		}

		if (-pointInBody.z > maxDotProd) {
			maxDotProd = -pointInBody.z;
			id = 5u;
		}

		return id;
	}

	Plane Ball::getPlaneFromSurface(const size_t surfaceId) const {
		switch (surfaceId) {
		case 0u:
		default: {
			Vector3 normal(1.0f, 0.0f, 0.0f);
			Plane aPlane(normal, realRadius);

			return aPlane;
		}
		case 1u: {
			Vector3 normal(0.0f, 1.0f, 0.0f);
			Plane aPlane(normal, realRadius);

			return aPlane;
		}
		case 2u: {
			Vector3 normal(0.0f, 0.0f, 1.0f);
			Plane aPlane(normal, realRadius);

			return aPlane;
		}
		case 3u: {
			Vector3 normal(-1.0f, 0.0f, 0.0f);
			Plane aPlane(normal, realRadius);

			return aPlane;
		}
		case 4u: {
			Vector3 normal(0.0f, -1.0f, 0.0f);
			Plane aPlane(normal, realRadius);

			return aPlane;
		}
		case 5u: {
			Vector3 normal(0.0f, 0.0f, -1.0f);
			Plane aPlane(normal, realRadius);

			return aPlane;
		}
		}
	}

	Vector3 Ball::getSurfaceNormalInBody(const size_t surfaceId) const {
		switch (surfaceId) {
		case 0u:
		default: {
			Vector3 normal(1.0f, 0.0f, 0.0f);

			return normal;
		}
		case 1u: {
			Vector3 normal(0.0f, 1.0f, 0.0f);

			return normal;
		}
		case 2u: {
			Vector3 normal(0.0f, 0.0f, 1.0f);

			return normal;
		}
		case 3u: {
			Vector3 normal(-1.0f, 0.0f, 0.0f);

			return normal;
		}
		case 4u: {
			Vector3 normal(0.0f, -1.0f, 0.0f);

			return normal;
		}
		case 5u: {
			Vector3 normal(0.0f, 0.0f, -1.0f);

			return normal;
		}
		}
	}

	Vector3 Ball::getSurfaceVertInBody(const size_t surfaceId, const size_t vertId) const {
		float x, y, z;

		switch (surfaceId) {
		case 0u:
		default: {
			switch (vertId) {
			case 0u:
			default: {
				x = realRadius; y = -realRadius; z = realRadius;

				break;
			}
			case 1u: {
				x = realRadius; y = -realRadius; z = -realRadius;

				break;
			}
			case 2u: {
				x = realRadius; y = realRadius; z = realRadius;

				break;
			}
			case 3u: {
				x = realRadius; y = realRadius; z = -realRadius;

				break;
			}
			}
			Vector3 virtualVertex(x, y, z);

			return virtualVertex;
		}
		case 1u: {
			switch (vertId) {
			case 0u:
			default: {
				x = realRadius; y = realRadius; z = -realRadius;

				break;
			}
			case 1u: {
				x = -realRadius; y = realRadius; z = -realRadius;

				break;
			}
			case 2u: {
				x = -realRadius; y = realRadius; z = realRadius;

				break;
			}
			case 3u: {
				x = realRadius; y = realRadius; z = realRadius;

				break;
			}
			}
			Vector3 virtualVertex(x, y, z);

			return virtualVertex;
		}
		case 2: {
			switch (vertId) {
			case 0u:
			default: {
				x = -realRadius; y = realRadius; z = realRadius;

				break;
			}
			case 1u: {
				x = -realRadius; y = -realRadius; z = realRadius;

				break;
			}
			case 2u: {
				x = realRadius; y = -realRadius; z = realRadius;

				break;
			}
			case 3u: {
				x = realRadius; y = realRadius; z = realRadius;

				break;
			}
			}
			Vector3 virtualVertex(x, y, z);

			return virtualVertex;
		}
		case 3u: {
			switch (vertId) {
			case 0u:
			default: {
				x = -realRadius; y = realRadius; z = -realRadius;

				break;
			}
			case 1u: {
				x = -realRadius; y = -realRadius; z = -realRadius;

				break;
			}
			case 2u: {
				x = -realRadius; y = -realRadius; z = realRadius;

				break;
			}
			case 3u: {
				x = -realRadius; y = realRadius; z = realRadius;

				break;
			}
			}
			Vector3 virtualVertex(x, y, z);

			return virtualVertex;
		}
		case 4u: {
			switch (vertId) {
			case 0u:
			default: {
				x = -realRadius; y = -realRadius; z = realRadius;

				break;
			}
			case 1u: {
				x = -realRadius; y = -realRadius; z = -realRadius;

				break;
			}
			case 2u: {
				x = realRadius; y = -realRadius; z = -realRadius;

				break;
			}
			case 3u: {
				x = realRadius; y = -realRadius; z = realRadius;

				break;
			}
			}
			Vector3 virtualVertex(x, y, z);

			return virtualVertex;
		}
		case 5u: {
			switch (vertId) {
			case 0u:
			default: {
				x = realRadius; y = -realRadius; z = -realRadius;

				break;
			}
			case 1u: {
				x = -realRadius; y = -realRadius; z = -realRadius;

				break;
			}
			case 2u: {
				x = -realRadius; y = -realRadius; z = realRadius;

				break;
			}
			case 3u: {
				x = realRadius; y = -realRadius; z = realRadius;

				break;
			}
			}
			Vector3 virtualVertex(x, y, z);

			return virtualVertex;
		}
		}

	}

	size_t Ball::getMostAlignedSurface(const Vector3& vecInWorld, const G3D::Matrix3& objectR) const {
		size_t id = 0u;
		Vector3 pointInBody = objectR.transpose() * vecInWorld;
		float maxDotProd = pointInBody.x;

		if (pointInBody.y > maxDotProd) {
			maxDotProd = pointInBody.y;
			id = 1u;
		}

		if (pointInBody.z > maxDotProd) {
			maxDotProd = pointInBody.z;
			id = 2u;
		}

		if (-pointInBody.x > maxDotProd) {
			maxDotProd = -pointInBody.x;
			id = 3u;
		}

		if (-pointInBody.y > maxDotProd) {
			maxDotProd = -pointInBody.y;
			id = 4u;
		}

		if (-pointInBody.z > maxDotProd) {
			maxDotProd = -pointInBody.z;
			id = 5u;
		}

		return id;
	}

	size_t Ball::getNumVertsInSurface(const size_t surfaceId) const {
		return 4u;
	}

	bool Ball::vertOverlapsFace(const Vector3& pointInBody, const size_t surfaceId) const {
		switch (surfaceId) {
		case 0u:
		case 3u:
		default: {
			if (fabs(pointInBody.y) < realRadius && fabs(pointInBody.z) < realRadius)
				return true;
		}
		case 1u:
		case 4u: {
			if (fabs(pointInBody.x) < realRadius && fabs(pointInBody.z) < realRadius)
				return true;
		}
		case 2u:
		case 5u: {
			if (fabs(pointInBody.x) < realRadius && fabs(pointInBody.y) < realRadius)
				return true;
		}
		}

		return false;
	}

	CoordinateFrame Ball::getSurfaceCoordInBody(const size_t surfaceId) const {
		// This computes the CS for the specified surface.  It is expressed in terms of the body, not world.
		// The surface centroid is the origin and the frame is aligned with the surface normal (for z).  The y axis of
		// this frame is the projection of either the body's y or z axis, depending on which one has a more predominant projection.
		CoordinateFrame aCS;

		// Compute and set centroid
		Vector3 faceCentroid(0.0f, 0.0f, 0.0f);

		switch (surfaceId) {
		case 0u:
		default:
			faceCentroid.x = realRadius;
			break;
		case 1u:
			faceCentroid.y = realRadius;
			break;
		case 2u:
			faceCentroid.z = realRadius;
			break;
		case 3u:
			faceCentroid.x = -realRadius;
			break;
		case 4u:
			faceCentroid.y = -realRadius;
			break;
		case 5u:
			faceCentroid.z = -realRadius;
			break;
		}

		aCS.translation = faceCentroid;
		aCS.rotation = Math::getWellFormedRotForZVector(getSurfaceNormalInBody(surfaceId));

		return aCS;
	}

	bool Ball::setUpBulletCollisionData(void) {
		if (!bulletCollisionObject)
			updateBulletCollisionData();

		return true;
	}

	void Ball::updateBulletCollisionData() {
		if (!bulletCollisionObject)
			bulletCollisionObject.reset(new btCollisionObject());

		bulletSphereShape = BulletSphereShapePool::getToken(getSize().x);
		bulletCollisionObject->setCollisionShape(const_cast<btSphereShape*>(bulletSphereShape->getShape()));
	}

} // namespace RBX
