#include "stdafx.h"

#include "V8World/Poly.h"
#include "V8World/Mesh.h"
#include "Util/Math.h"
#include "V8World/Tolerance.h"



namespace RBX {

	bool Poly::hitTest(const Ray& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal) {
		return mesh->hitTest(rayInMe, localHitPoint, surfaceNormal);
	}

	bool Poly::collidesWithGroundPlane(const CoordinateFrame& c, float yHeight) const {
		for (size_t i = 0u; i < mesh->numVertices(); ++i) {
			Vector3 worldLocation = c.pointToWorldSpace(mesh->getVertex(i)->getOffset());

			if (worldLocation.y < yHeight)
				return true;
		}
		return false;
	}

	// See scanned calulations in V8 Technical Doc
	Matrix3 Poly::getMoment(float mass) const {
		Vector3 size = getSize();

		float area = 2.0f * (size.x * size.y + size.y * size.z + size.z * size.x);
		Vector3 I;

		for (size_t i = 0u; i < 3u; i++) {
			size_t j = (i + 1u) % 3u;
			size_t k = (i + 2u) % 3u;
			float x = size[i];		// main axis;
			float y = size[j];
			float z = size[k];

			float Ix = (mass / (2.0f * area)) * ((y * y * y * z / 3.0f)
				+ (y * z * z * z / 3.0f)
				+ (x * y * z * z)
				+ (x * y * y * y / 3.0f)
				+ (x * y * y * z)
				+ (x * z * z * z / 3.0f));

			I[i] = Ix;
		}
		return Math::fromDiagonal(I);
	}

	Vector3 Poly::getCofmOffset(void) const {
		return Vector3::zero();
	}

	void Poly::setSize(const G3D::Vector3& _size) {
		Super::setSize(_size);
		RBXASSERT(_size == getSize());

		centerToCornerDistance = 0.5f * _size.magnitude();

		buildMesh();
	}

	size_t Poly::closestSurfaceToPoint(const Vector3& pointInBody) const {
		// Return the first face index where pointInBody is in and above the face.
		size_t faceId = (size_t)-1;
		float minFaceDist = 1e6f;

		for (size_t i = 0u; i < mesh->numFaces(); i++) {
			bool inFace = mesh->getFace(i)->pointInExtrusion(pointInBody);

			if (inFace) {
				float curFaceDist = fabs(mesh->getFace(i)->plane().distance(pointInBody));

				if (curFaceDist < minFaceDist) {
					minFaceDist = curFaceDist;
					faceId = i;
				}
			}
		}

		RBXASSERT(faceId != (size_t)-1);

		return faceId;
	}

	Plane Poly::getPlaneFromSurface(const size_t surfaceId) const {
		RBXASSERT(surfaceId != (size_t)-1);

		return surfaceId != (size_t)-1 ? mesh->getFace(surfaceId)->plane() : Plane();
	}

	Vector3 Poly::getSurfaceNormalInBody(const size_t surfaceId) const {
		RBXASSERT(surfaceId != (size_t)-1);

		return surfaceId != (size_t)-1 ? mesh->getFace(surfaceId)->normal() : Vector3(0, 0, 0);
	}

	Vector3 Poly::getSurfaceVertInBody(const size_t surfaceId, const size_t vertId) const {
		RBXASSERT(surfaceId != (size_t)-1);

		return surfaceId != (size_t)-1 ? mesh->getFace(surfaceId)->getVertex(vertId)->getOffset() : Vector3(0, 0, 0);
	}

	size_t Poly::getMostAlignedSurface(const Vector3& vecInWorld, const G3D::Matrix3& objectR) const {
		size_t faceId = (size_t)-1;
		float maxDotProd = 0.0f;
		Vector3 vecInBody = objectR.transpose() * vecInWorld;

		for (size_t i = 0u; i < mesh->numFaces(); i++) {
			float currentDotProd = mesh->getFace(i)->normal().dot(vecInBody);

			if (currentDotProd > maxDotProd) {
				maxDotProd = currentDotProd;
				faceId = i;
			}
		}

		RBXASSERT(faceId != (size_t)-1);

		return faceId;
	}

	size_t Poly::getNumVertsInSurface(const size_t surfaceId) const {
		RBXASSERT(surfaceId != (size_t)-1);

		return surfaceId != (size_t)-1 ? mesh->getFace(surfaceId)->numVertices() : 0u;
	}

	bool Poly::vertOverlapsFace(const Vector3& pointInBody, const size_t surfaceId) const {
		RBXASSERT(surfaceId != (size_t)-1);

		return surfaceId != (size_t)-1 ? mesh->getFace(surfaceId)->pointInExtrusion(pointInBody) : false;
	}

	CoordinateFrame Poly::getSurfaceCoordInBody(const size_t surfaceId) const {
		RBXASSERT(surfaceId != (size_t)-1);

		CoordinateFrame aCS;
		if (surfaceId == (size_t)-1)
			return aCS;

		// the face reference coord origin is the midpoint b/t the 0 and 1 vertex
		aCS.translation = 0.5f * (mesh->getFace(surfaceId)->getVertex(0)->getOffset() + mesh->getFace(surfaceId)->getVertex(1)->getOffset());
		aCS.rotation = Math::getWellFormedRotForZVector(mesh->getFace(surfaceId)->normal());

		return aCS;
	}

	std::vector<Vector3> Poly::polygonIntersectionWithFace(const std::vector<Vector3>& otherPolygonInBody, const size_t surfaceId) const {
		RBXASSERT(surfaceId != (size_t)-1);

		std::vector<Vector3> intersection3d;
		if (surfaceId == (size_t)-1)
			return intersection3d;

		std::vector<Vector2> myPolygon, otherPolygon;

		CoordinateFrame myC = getSurfaceCoordInBody(surfaceId);

		// represent the other polygon and this polyhrons face polygon in 2D coordinates.
		for (size_t i = 0u; i < getNumVertsInSurface(surfaceId); i++) {
			Vector3 myVertInBody = getSurfaceVertInBody(surfaceId, i);
			Vector3 myVertInSurface = myC.rotation.transpose() * (myVertInBody - myC.translation);
			myPolygon.push_back(myVertInSurface.xy());
		}

		for (size_t i = 0u; i < otherPolygonInBody.size(); i++) {
			Vector3 otherVertInBody = otherPolygonInBody[i];
			Vector3 otherVertInSurface = myC.rotation.transpose() * (otherVertInBody - myC.translation);
			otherPolygon.push_back(otherVertInSurface.xy());
		}

		// find intersection
		std::vector<Vector2> intersection2d = Math::planarPolygonIntersection(myPolygon, otherPolygon);

		// convert result to 3d and transform from surface coord to body coord
		for (size_t i = 0u; i < intersection2d.size(); i++) {
			Vector3 intersectionVertInSurface(intersection2d[i].x, intersection2d[i].y, 0.0f);
			Vector3 intersectionVertInBody = myC.translation + (myC.rotation * intersectionVertInSurface);
			intersection3d.push_back(intersectionVertInBody);
		}

		return intersection3d;
	}

	bool Poly::findTouchingSurfacesConvex(const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId) const
	{
		// find the touching surfaces
		// for each pair of surfaces, check passing of these required conditions
		// 1. parallel
		// 2. overlapping
		// 3. within distance tolerance

		for (myFaceId = 0u; myFaceId < (size_t)getNumSurfaces(); myFaceId++) {
			for (otherFaceId = 0u; otherFaceId < (size_t)otherGeom.getNumSurfaces(); otherFaceId++) {
				CoordinateFrame face0Coord = myCf * getSurfaceCoordInBody(myFaceId);
				CoordinateFrame face1Coord = otherCf * otherGeom.getSurfaceCoordInBody(otherFaceId);

				Vector3 p0ZInWorld = face0Coord.vectorToWorldSpace(Vector3::unitZ());
				Vector3 p0ZInP1 = face1Coord.vectorToObjectSpace(p0ZInWorld);
				if (fabs(1.0f + p0ZInP1.dot(Vector3::unitZ())) > Tolerance::jointAngleMax())
					continue;

				if (!FacesOverlapped(myCf, myFaceId, otherGeom, otherCf, otherFaceId, 0.99f))
					continue;

				//if distance b/t faces is outside tolerance
				//Vector3 face0OriginWorld = face0Coord.translation;
				Vector3 face1OriginWorld = face1Coord.translation;
				Vector3 face1OriginInface0Coord = face0Coord.pointToObjectSpace(face1OriginWorld);

				// if the z distance (in face0 coord system) of the face1 origin is more than the distance tolerance, return false.
				// Scale tolerance up by factor of two for special shapes due to less precise face alignment.
				if (fabs(face1OriginInface0Coord.z) > 2.0f * Tolerance::jointPlanarMax())
					continue;

				// found a contacting pair	
				return true;
			}
		}

		// Nothing found
		return false;
	}

	bool Poly::FacesOverlapped(const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol) const {
		// The input "adjustPartTolerance" is used to more liberally or conservatively detect overlap.
		// It has the effect of slightly shrinking or expanding a part to more/less readily get overlap.
		// It defaults to 1.0.

		if (FaceVerticesOverlapped(myCf, myFaceId, otherGeom, otherCf, otherFaceId, tol))
			return true;

		if (FaceEdgesOverlapped(myCf, myFaceId, otherGeom, otherCf, otherFaceId, tol))
			return true;

		// no overlap found.
		return false;
	}

	bool Poly::FaceVerticesOverlapped(const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol) const {
		// The input "adjustPartTolerance" is used to more liberally or conservatively detect overlap.
		// It has the effect of slightly shrinking or expanding a part to more/less readily get overlap.
		size_t numVertsInface0 = getNumVertsInSurface(myFaceId);
		for (size_t i = 0u; i < numVertsInface0; ++i) {
			Vector3 p0VertInp0 = getSurfaceVertInBody(myFaceId, i) * tol;
			Vector3 p0VertInWorld = myCf.pointToWorldSpace(p0VertInp0);
			Vector3 p0VertInp1 = otherCf.pointToObjectSpace(p0VertInWorld);

			if (otherGeom.vertOverlapsFace(p0VertInp1, otherFaceId))
				return true;
		}

		// Now test snap verts in drag face
		size_t numVertsInface1 = otherGeom.getNumVertsInSurface(otherFaceId);
		for (size_t i = 0u; i < numVertsInface1; ++i) {
			Vector3 p1VertInp1 = otherGeom.getSurfaceVertInBody(otherFaceId, i) * tol;
			Vector3 p1VertInWorld = otherCf.pointToWorldSpace(p1VertInp1);
			Vector3 p1VertInp0 = myCf.pointToObjectSpace(p1VertInWorld);

			if (vertOverlapsFace(p1VertInp0, myFaceId))
				return true;
		}

		return false;
	}

	bool Poly::FaceEdgesOverlapped(const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol) const {
		// The input "adjustPartTolerance" is used to more liberally or conservatively detect overlap.
		// It has the effect of slightly shrinking or expanding a part to more/less readily get overlap.
		const float distanceTolerance = 1e-5;
		const float largeDistance = 1e6;
		size_t numVertsInface0 = getNumVertsInSurface(myFaceId);
		size_t numVertsInface1 = otherGeom.getNumVertsInSurface(otherFaceId);

		for (size_t i = 0u; i < numVertsInface0 - 1u; ++i) {
			// line segment from A to B
			Vector3 face0VertInp0_A = getSurfaceVertInBody(myFaceId, i);
			Vector3 face0VertInWorld_A = myCf.pointToWorldSpace(face0VertInp0_A);
			Vector3 face0VertInp1_A = otherCf.pointToObjectSpace(face0VertInWorld_A);
			Vector3 face0VertInp0_B = getSurfaceVertInBody(myFaceId, i + 1u);
			Vector3 face0VertInWorld_B = myCf.pointToWorldSpace(face0VertInp0_B);
			Vector3 face0VertInp1_B = otherCf.pointToObjectSpace(face0VertInWorld_B);

			for (size_t j = 0u; j < numVertsInface1 - 1u; ++j) {
				Vector3 face1VertInp1_A = otherGeom.getSurfaceVertInBody(otherFaceId, j);
				Vector3 face1VertInp1_B = otherGeom.getSurfaceVertInBody(otherFaceId, j + 1u);

				float distance = largeDistance;
				bool crossing = Math::lineSegmentDistanceIfCrossing(face0VertInp1_A, face0VertInp1_B, face1VertInp1_A, face1VertInp1_B, distance, tol);

				if (crossing && distance < distanceTolerance)
					return true;
			}

			// Check last pair of p1-face1 verts
			Vector3 face1VertInp1_A = otherGeom.getSurfaceVertInBody(otherFaceId, numVertsInface1 - 1u);
			Vector3 face1VertInp1_B = otherGeom.getSurfaceVertInBody(otherFaceId, 0u);

			float distance = largeDistance;
			bool crossing = Math::lineSegmentDistanceIfCrossing(face0VertInp1_A, face0VertInp1_B, face1VertInp1_A, face1VertInp1_B, distance, tol);

			if (crossing && distance < distanceTolerance)
				return true;
		}

		// Now check last pair of p0-face0 verts with all pairs of p1-face1 verts.
		Vector3 face0VertInp0_A = getSurfaceVertInBody(myFaceId, numVertsInface0 - 1u);
		Vector3 face0VertInWorld_A = myCf.pointToWorldSpace(face0VertInp0_A);
		Vector3 face0VertInp1_A = otherCf.pointToObjectSpace(face0VertInWorld_A);
		Vector3 face0VertInp0_B = getSurfaceVertInBody(myFaceId, 0u);
		Vector3 face0VertInWorld_B = myCf.pointToWorldSpace(face0VertInp0_B);
		Vector3 face0VertInp1_B = otherCf.pointToObjectSpace(face0VertInWorld_B);

		for (size_t j = 0u; j < numVertsInface1 - 1u; ++j) {
			Vector3 face1VertInp1_A = otherGeom.getSurfaceVertInBody(otherFaceId, j);
			Vector3 face1VertInp1_B = otherGeom.getSurfaceVertInBody(otherFaceId, j + 1u);

			float distance = largeDistance;
			bool crossing = Math::lineSegmentDistanceIfCrossing(face0VertInp1_A, face0VertInp1_B, face1VertInp1_A, face1VertInp1_B, distance, tol);

			if (crossing && distance < distanceTolerance)
				return true;
		}

		// Check last pair of p1-face1 verts
		Vector3 face1VertInp1_A = otherGeom.getSurfaceVertInBody(otherFaceId, numVertsInface1 - 1u);
		Vector3 face1VertInp1_B = otherGeom.getSurfaceVertInBody(otherFaceId, 0u);

		float distance = largeDistance;
		bool crossing = Math::lineSegmentDistanceIfCrossing(face0VertInp1_A, face0VertInp1_B, face1VertInp1_A, face1VertInp1_B, distance, tol);

		if (crossing && distance < distanceTolerance)
			return true;

		// no overlap found.
		return false;
	}

} // namespace RBX