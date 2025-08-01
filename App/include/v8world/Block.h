#pragma once

#include "V8World/Poly.h"
#include "V8World/GeometryPool.h"
#include "V8World/BlockCorners.h"
#include "V8World/BlockMesh.h"
#include "V8World/BulletGeometryPoolObjects.h"

#include "V8Kernel/ContactParams.h"
#include "Util/NormalID.h"
#include "rbx/Debug.h"

namespace RBX {

	class Block : public Poly {
		friend class TriangleMesh;
	private:
		typedef Poly Super;
	public:
		typedef GeometryPool<Vector3, POLY::BlockMesh, Vector3Comparer> BlockMeshPool;
		typedef GeometryPool<Vector3, POLY::BlockCorners, Vector3Comparer> BlockCornersPool;
		typedef GeometryPool<Vector3, BulletBoxShapeWrapper, Vector3Comparer> BulletBoxShapePool;
	private:
		BlockCornersPool::Token blockCorners;
		BlockMeshPool::Token blockMesh;
		BulletBoxShapePool::Token bulletBoxShape;

		const Vector3* vertices;	// in Real World units, object coords - shortcut to wrapper data

		static const uint32_t BLOCK_FACE_TO_VERTEX[6][4];
		static const uint32_t BLOCK_FACE_VERTEX_TO_EDGE[6][4];

		// loading GeoPair stuff
		const Vector3* getCornerPoint(const Vector3int16& clip) const;
		const Vector3* getEdgePoint(const Vector3int16& clip, NormalId& normalID) const;
		const Vector3* getPlanePoint(const Vector3int16& clip, NormalId& normalID) const;

		Matrix3 getMomentHollow(float mass) const;

		/*override*/ void setSize(const G3D::Vector3& _size);

		// Primitive Overrides
		/*override*/ virtual bool hitTest(const RbxRay& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal);

		/*override*/ virtual GeometryType getGeometryType() const { return GEOMETRY_BLOCK; }
		/*override*/ virtual CollideType getCollideType() const { return COLLIDE_BLOCK; }

	public:
		// Real Corner
		/*override*/ virtual Vector3 getCenterToCorner(const Matrix3& rotation) const;

	private:
		// Moment
		/*override*/ virtual Matrix3 getMoment(float mass) const { return getMomentHollow(mass); }

		// Volume
		/*override*/ float getVolume() const;

		// Poly Overrides
		/*override*/ void buildMesh();

		void updateBulletCollisionData();

	public:
		Block() : vertices(nullptr) {}

		~Block() {}

		static void init();

		///////////////////////////////////////////////////////////////
		// 
		// Block Specific Collision Detection
		void projectToFace(Vector3& ray, Vector3int16& clip, uint32_t& onBorder);

		GeoPairType getBallInsideInfo(const Vector3& ray, const Vector3*& offset, NormalId& normalID);
		GeoPairType getBallBlockInfo(uint32_t onBorder, const Vector3int16 clip, const Vector3*& offset, NormalId& normalID);

		inline const float* getVertices() const {
			return (float*)vertices;
		}

		inline const Vector3& getExtent() const {
			return vertices[0];
		}

		const Vector3* getFaceVertex(NormalId faceID, uint32_t vertID) const {
			return &vertices[BLOCK_FACE_TO_VERTEX[faceID][vertID]];
		}

		uint32_t getClosestEdge(const Matrix3& rotation, NormalId normalID, const Vector3& crossAxis);

		// tricky - given a face and a vertex on it, find the edge
		// assumes that the vertices are in counterclockwise order on the face,
		// and gives the edge that connects this vertex with the next one in 
		// counter-clockwise order
		inline uint32_t faceVertexToEdge(NormalId faceID, uint32_t vertID) {
			return BLOCK_FACE_VERTEX_TO_EDGE[faceID][vertID];
		}

		// same as the previsous, but gives the edge that
		// connects with the next in clockwise order
		inline uint32_t faceVertexToClockwiseEdge(NormalId faceID, uint32_t vertID) {
			return 12u + BLOCK_FACE_VERTEX_TO_EDGE[faceID][vertID];
		}

		const Vector3* getEdgeVertex(uint32_t edgeId) const {
			if (edgeId < 12u) {
				return &vertices[Block::BLOCK_FACE_TO_VERTEX[edgeId / 4u][edgeId % 4u]];
			}
			else {
				uint32_t ccwEdge = edgeId - 12u;		// convert to regular..
				NormalId faceId = (NormalId)(ccwEdge / 4u);

				RBXASSERT(validNormalId(faceId));

				uint32_t vertId = ccwEdge + 1u % 4u;		// one higher - add

				return &vertices[Block::BLOCK_FACE_TO_VERTEX[faceId][vertId]];
			}
		}

		// returns X,-X,X,-X,Y,-Y,Y,-Y,Z,-Z,Z,-Z
		inline NormalId getEdgeNormal(uint32_t edgeId) {
			NormalId ans = static_cast<NormalId>((edgeId / 4u) + (3u * (edgeId % 2u)));

			if (edgeId > 12u) {
				ans = static_cast<NormalId>((ans + 3u) % 6u);
			}

			return ans;
		}

		Vector2 getProjectedVertex(const Vector3& vertex, NormalId normalID);

		// Currently used by dragger
		/*override*/ CoordinateFrame getSurfaceCoordInBody(const size_t surfaceId) const;

		/*override*/ bool setUpBulletCollisionData(void);
	};

} // namespace
