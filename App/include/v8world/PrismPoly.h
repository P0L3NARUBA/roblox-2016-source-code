#pragma once

#include "V8World/Poly.h"
#include "V8World/GeometryPool.h"
#include "V8World/PrismMesh.h"
#include "V8World/BlockMesh.h"

namespace RBX {

	class PrismPoly : public Poly {
	private:
		typedef GeometryPool<Vector3_2Ints, POLY::PrismMesh, Vector3_2IntsComparer> PrismMeshPool;
		PrismMeshPool::Token prismMesh;

		size_t numSides;
		size_t numSlices;

		void setNumSides(size_t num);
		void setNumSlices(size_t num);
		/*override*/ bool isGeometryOrthogonal(void) const { return false; }

	protected:
		// Geometry Overrides
		/*override*/ virtual GeometryType getGeometryType() const { return GEOMETRY_PRISM; }
		/*override*/ void setGeometryParameter(const std::string& parameter, size_t value);
		/*override*/ size_t getGeometryParameter(const std::string& parameter) const;

		/*override*/ Matrix3 getMoment(float mass) const;
		/*override*/ Vector3 getCofmOffset() const;
		/*override*/ CoordinateFrame getSurfaceCoordInBody(const size_t surfaceId) const;
		/*override*/ size_t getFaceFromLegacyNormalId(const NormalId nId) const;

		// Poly Overrides
		/*override*/ void buildMesh();

	public:
		PrismPoly() : numSides(0u), numSlices(0u)
		{
		}

		/*override*/ bool setUpBulletCollisionData(void) { return false; }

	};

} // namespace
