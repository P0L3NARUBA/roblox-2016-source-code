#include "stdafx.h"

#include "V8World/Mesh.h"
#include "util/Math.h"

namespace RBX {

	namespace POLY {
		/*
			WEDGE

			Back:	Looking along -z
				^ y
			3	0
			4	1	> x

			Front:	Looking along z
				^ y
			0	3
			2	5	> -x

			Right: Looking along -x
				^ y
			0
			1	2	> -z

			Left: Looking along x
				^ y
				3
			5	4	> z

			Bottom: Looking along -y
				^ -z
			2	5
			1	4	> -x

			Top: Looking along -y
				^ -z
			5	2
			3	0	> x

		*/

		/*
			BLOCK

		vertID		x,y,z
		0			1,1,1
		1			1,1,-1
		2			1,-1,1
		3			1,-1,-1
		4			-1,1,1
		5			-1,1,-1
		6			-1,-1,1
		7			-1,-1,-1

		0			+x
		1			+y
		2			+z
		3			-x
		4			-y
		5			-z
			Right: Looking at x
				^ y
			0	1
			2	3	> -z

			Top: Looking at y
				^ -z
			5	1
			4	0	> x

			Back:	Looking at z
				^ y
			4	0
			6	2	> x

			Left: Looking at -x
				^ y
			5	4
			7	6	> z

			Bottom: Looking at -y
				^ -z
			3	7
			2   6	> -x

			Front:	Looking at -z
				^ y
			1	5
			3	7	> -x
		*/

		void Mesh::makeBlock(const Vector3& size) {
			float x = 0.5f * size.x;
			float y = 0.5f * size.y;
			float z = 0.5f * size.z;

			clear();

			// important - don't reallocate while we are building
			vertices.reserve(8u);
			faces.reserve(6u);
			edges.reserve(12u);

			addVertex( x,  y,  z);
			addVertex( x,  y, -z);
			addVertex( x, -y,  z);
			addVertex( x, -y, -z);
			addVertex(-x,  y,  z);
			addVertex(-x,  y, -z);
			addVertex(-x, -y,  z);
			addVertex(-x, -y, -z);

			// counter clockwise
			addFace(1u, 0u, 2u, 3u);	// right
			addFace(1u, 5u, 4u, 0u);	// top
			addFace(0u, 4u, 6u, 2u);	// back
			addFace(4u, 5u, 7u, 6u);	// left
			addFace(7u, 3u, 2u, 6u);	// bottom
			addFace(5u, 1u, 3u, 7u);	// front
		}


		void Mesh::makeCell(const Vector3& size, const Vector3& offset) {
			float x = 0.5f * size.x;
			float y = 0.5f * size.y;
			float z = 0.5f * size.z;

			float sumx = x + offset.x;
			float diffx = offset.x - x;
			float sumy = y + offset.y;
			float diffy = offset.y - y;
			float sumz = z + offset.z;
			float diffz = offset.z - z;

			clear();

			// important - don't reallocate while we are building
			vertices.reserve(8u);
			faces.reserve(6u);
			edges.reserve(12u);

			addVertex(sumx,  sumy,  sumz );
			addVertex(sumx,  sumy,  diffz);
			addVertex(sumx,  diffy, sumz );
			addVertex(sumx,  diffy, diffz);
			addVertex(diffx, sumy,  sumz );
			addVertex(diffx, sumy,  diffz);
			addVertex(diffx, diffy, sumz );
			addVertex(diffx, diffy, diffz);

			// counter clockwise
			addFace(1u, 0u, 2u, 3u);	// right
			addFace(1u, 5u, 4u, 0u);	// top
			addFace(0u, 4u, 6u, 2u);	// back
			addFace(4u, 5u, 7u, 6u);	// left
			addFace(7u, 3u, 2u, 6u);	// bottom
			addFace(5u, 1u, 3u, 7u);	// front
		}

		void Mesh::makeVerticalWedgeCell(const Vector3& size, const Vector3& offset, const size_t& orient) {
			float x = 0.5f * size.x;
			float y = 0.5f * size.y;
			float z = 0.5f * size.z;

			float sumx = x + offset.x;
			float diffx = offset.x - x;
			float sumy = y + offset.y;
			float diffy = offset.y - y;
			float sumz = z + offset.z;
			float diffz = offset.z - z;

			clear();

			// important - don't reallocate while we are building
			vertices.reserve(6u);
			faces.reserve(5u);
			edges.reserve(9u);

			// add wedge base verts
			addVertex(sumx, diffy, sumz);
			addVertex(sumx, diffy, diffz);
			addVertex(diffx, diffy, diffz);
			addVertex(diffx, diffy, sumz);
			addFace(0u, 3u, 2u, 1u);

			switch (orient) {
			case 0u: // UpperLeft
			default:
				addVertex(sumx, sumy, diffz);
				addVertex(diffx, sumy, diffz);
				addFace(0u, 4u, 5u, 3u);
				addFace(1u, 2u, 5u, 4u);
				addFace(1u, 4u, 0u);
				addFace(2u, 3u, 5u);

				break;
			case 1u: // UpperRight
				addVertex(diffx, sumy, diffz);
				addVertex(diffx, sumy, sumz);
				addFace(4u, 2u, 3u, 5u);
				addFace(0u, 1u, 4u, 5u);
				addFace(0u, 5u, 3u);
				addFace(2u, 4u, 1u);

				break;
			case 2u: // LowerRight
				addVertex(diffx, sumy, sumz);
				addVertex(sumx, sumy, sumz);
				addFace(0u, 5u, 4u, 3u);
				addFace(1u, 2u, 4u, 5u);
				addFace(3u, 4u, 2u);
				addFace(0u, 1u, 5u);

				break;
			case 3u: // LowerLeft
				addVertex(sumx, sumy, sumz);
				addVertex(sumx, sumy, diffz);
				addFace(0u, 1u, 5u, 4u);
				addFace(3u, 4u, 5u, 2u);
				addFace(0u, 4u, 3u);
				addFace(1u, 2u, 5u);

				break;
			}
		}

		void Mesh::makeHorizontalWedgeCell(const Vector3& size, const Vector3& offset, const size_t& orient) {
			float x = 0.5f * size.x;
			float y = 0.5f * size.y;
			float z = 0.5f * size.z;

			float sumx = x + offset.x;
			float diffx = offset.x - x;
			float sumy = y + offset.y;
			float diffy = offset.y - y;
			float sumz = z + offset.z;
			float diffz = offset.z - z;

			clear();

			// important - don't reallocate while we are building
			vertices.reserve(6u);
			faces.reserve(5u);
			edges.reserve(9u);

			switch (orient) {
			case 0u: // UpperLeft
			default:
				addVertex(sumx, diffy, sumz);
				addVertex(sumx, diffy, diffz);
				addVertex(diffx, diffy, diffz);
				addVertex(sumx, sumy, sumz);
				addVertex(sumx, sumy, diffz);
				addVertex(diffx, sumy, diffz);

				break;
			case 1u: // UpperRight
				addVertex(sumx, diffy, diffz);
				addVertex(diffx, diffy, diffz);
				addVertex(diffx, diffy, sumz);
				addVertex(sumx, sumy, diffz);
				addVertex(diffx, sumy, diffz);
				addVertex(diffx, sumy, sumz);

				break;
			case 2u: // LowerRight
				addVertex(diffx, diffy, diffz);
				addVertex(diffx, diffy, sumz);
				addVertex(sumx, diffy, sumz);
				addVertex(diffx, sumy, diffz);
				addVertex(diffx, sumy, sumz);
				addVertex(sumx, sumy, sumz);

				break;
			case 3u: // LowerLeft
				addVertex(diffx, diffy, sumz);
				addVertex(sumx, diffy, sumz);
				addVertex(sumx, diffy, diffz);
				addVertex(diffx, sumy, sumz);
				addVertex(sumx, sumy, sumz);
				addVertex(sumx, sumy, diffz);

				break;
			}

			addFace(3u, 4u, 5u);
			addFace(2u, 1u, 0u);
			addFace(0u, 3u, 5u, 2u);
			addFace(0u, 1u, 4u, 3u);
			addFace(1u, 2u, 5u, 4u);
		}

		void Mesh::makeCornerWedgeCell(const Vector3& size, const Vector3& offset, const size_t& orient) {
			float x = 0.5f * size.x;
			float y = 0.5f * size.y;
			float z = 0.5f * size.z;

			float sumx = x + offset.x;
			float diffx = offset.x - x;
			float sumy = y + offset.y;
			float diffy = offset.y - y;
			float sumz = z + offset.z;
			float diffz = offset.z - z;

			clear();

			// important - don't reallocate while we are building
			vertices.reserve(4u);
			faces.reserve(4u);
			edges.reserve(6u);

			switch (orient) {
			case 0u: // UpperLeft
			default:
				addVertex(sumx, diffy, sumz);
				addVertex(sumx, diffy, diffz);
				addVertex(diffx, diffy, diffz);
				addVertex(sumx, sumy, diffz);

				break;
			case 1u: // UpperRight
				addVertex(sumx, diffy, diffz);
				addVertex(diffx, diffy, diffz);
				addVertex(diffx, diffy, sumz);
				addVertex(diffx, sumy, diffz);

				break;
			case 2u: // LowerRight
				addVertex(diffx, diffy, diffz);
				addVertex(diffx, diffy, sumz);
				addVertex(sumx, diffy, sumz);
				addVertex(diffx, sumy, sumz);

				break;
			case 3u: // LowerLeft
				addVertex(diffx, diffy, sumz);
				addVertex(sumx, diffy, sumz);
				addVertex(sumx, diffy, diffz);
				addVertex(sumx, sumy, sumz);

				break;
			}

			addFace(2u, 1u, 0u);
			addFace(0u, 1u, 3u);
			addFace(1u, 2u, 3u);
			addFace(0u, 3u, 2u);
		}

		void Mesh::makeInverseCornerWedgeCell(const Vector3& size, const Vector3& offset, const size_t& orient) {
			float x = 0.5f * size.x;
			float y = 0.5f * size.y;
			float z = 0.5f * size.z;

			float sumx = x + offset.x;
			float diffx = offset.x - x;
			float sumy = y + offset.y;
			float diffy = offset.y - y;
			float sumz = z + offset.z;
			float diffz = offset.z - z;

			clear();

			// important - don't reallocate while we are building
			vertices.reserve(7u);
			faces.reserve(7u);
			edges.reserve(12u);

			switch (orient) {
			case 0u: // UpperLeft
			default:
				addVertex(sumx, diffy, sumz);
				addVertex(sumx, diffy, diffz);
				addVertex(diffx, diffy, diffz);
				addVertex(diffx, diffy, sumz);
				addVertex(sumx, sumy, sumz);
				addVertex(sumx, sumy, diffz);
				addVertex(diffx, sumy, diffz);

				break;
			case 1u: // UpperRight
				addVertex(sumx, diffy, diffz);
				addVertex(diffx, diffy, diffz);
				addVertex(diffx, diffy, sumz);
				addVertex(sumx, diffy, sumz);
				addVertex(sumx, sumy, diffz);
				addVertex(diffx, sumy, diffz);
				addVertex(diffx, sumy, sumz);

				break;
			case 2u: // LowerRight
				addVertex(diffx, diffy, diffz);
				addVertex(diffx, diffy, sumz);
				addVertex(sumx, diffy, sumz);
				addVertex(sumx, diffy, diffz);
				addVertex(diffx, sumy, diffz);
				addVertex(diffx, sumy, sumz);
				addVertex(sumx, sumy, sumz);

				break;
			case 3u: // LowerLeft
				addVertex(diffx, diffy, sumz);
				addVertex(sumx, diffy, sumz);
				addVertex(sumx, diffy, diffz);
				addVertex(diffx, diffy, diffz);
				addVertex(diffx, sumy, sumz);
				addVertex(sumx, sumy, sumz);
				addVertex(sumx, sumy, diffz);

				break;
			}

			addFace(3u, 4u, 6u);
			addFace(3u, 6u, 2u);
			addFace(0u, 4u, 3u);
			addFace(0u, 1u, 5u, 4u);
			addFace(1u, 2u, 6u, 5u);
			addFace(0u, 3u, 2u, 1u);
			addFace(4u, 5u, 6u);
		}

		void Mesh::makeWedge(const Vector3& size) {
			float x = 0.5f * size.x;
			float y = 0.5f * size.y;
			float z = 0.5f * size.z;

			clear();

			// important - don't reallocate while we are building
			vertices.reserve(6u);
			faces.reserve(5u);
			edges.reserve(9u);

			addVertex(x, y, z);
			addVertex(x, -y, z);
			addVertex(x, -y, -z);
			addVertex(-x, y, z);
			addVertex(-x, -y, z);
			addVertex(-x, -y, -z);

			addFace(0u, 3u, 4u, 1u); // back
			addFace(3u, 0u, 2u, 5u); // front / top
			addFace(0u, 1u, 2u);	 // right
			addFace(3u, 5u, 4u);	 // left
			addFace(5u, 2u, 1u, 4u); // bottom
		}

		void Mesh::makePrism(const Vector3_2Ints& params, Vector3& cofm) {
			size_t sides = (size_t)params.int1;
			Vector3 size = params.vectPart;
			RBXASSERT(sides < 21u);

			// no assert for this case since it happens safely during initialization
			if (sides < 3u)
				sides = 6u;

			clear();

			// Set cofm to zero and recompute for this prism
			cofm = Vector3::zero();

			// Arrays of indices collected during construction of side walls to be used for end cap face creation.
			size_t* baseVertIndices = new size_t[sides];
			size_t* topVertIndices = new size_t[sides];

			vertices.reserve(2u * sides);
			faces.reserve(sides + 2u);
			edges.reserve(3u * sides);

			// Start creation of prism - this sets the maximum width, which is the x-width in properties
			float alpha = 180.0f / (float)sides;
			Vector2 startPoint = Math::polygonStartingPoint(sides, size.x);

			// Apply 2D startPoint from Math to 3D starting point for the polyhedron
			G3D::Vector3 Sa(startPoint.x, 0.0f, startPoint.y);
			G3D::Vector3 Sb(0.0f, 0.0f, 0.0f);
			G3D::Vector3 axis(0.0f, 1.0f, 0.0f);

			// start at bottom corner vertex for first side wall
			addVertex(startPoint.x, -0.5f * (float)size.y, startPoint.y);
			baseVertIndices[0] = 0u;

			addVertex(startPoint.x, 0.5f * (float)size.y, startPoint.y);
			topVertIndices[0] = 1u;

			cofm += Sa;

			// Walk perimeter of prism until last closing section is reached
			for (size_t i = 0u; i < sides - 1u; ++i) {
				G3D::Vector3 baseNorm = G3D::normalize(axis.cross(Sa));
				G3D::Vector3 SbRevNorm = G3D::normalize(-Sa);
				float effRad = Sa.magnitude();
				double sideLen = 2.0 * effRad * sin(0.0174533 * alpha);

				// update Sa to next facet
				Sb = Sa + sideLen * sin(0.0174533 * alpha) * SbRevNorm + sideLen * cos(0.0174533 * alpha) * baseNorm;
				cofm += Sb;
				Sa = Sb;

				addVertex(Sb.x, -0.5f * (float)size.y, Sb.z);
				baseVertIndices[i + 1u] = 2u * (i + 1u);

				addVertex(Sb.x, 0.5f * (float)size.y, Sb.z);
				topVertIndices[i + 1u] = 2u * (i + 1u) + 1u;

				addFace(2u * i, 2u * i + 2u, 2u * i + 3u, 2u * i + 1u);
			}

			// Finish off last side face
			addFace(2u * (sides - 1u), 0u, 1u, 2u * (sides - 1u) + 1u);
			cofm.x = cofm.y = cofm.z = 0.0f;

			// Now the base and top
			addFace(sides, topVertIndices, false);
			addFace(sides, baseVertIndices, true);

			delete[] topVertIndices;
			delete[] baseVertIndices;
		}

		void Mesh::makePyramid(const Vector3_2Ints& params, Vector3& cofm) {
			size_t sides = (size_t)params.int1;
			Vector3 size = params.vectPart;
			RBXASSERT(sides < 21u);

			// no assert for this case since it happens safely during initialization
			if (sides < 3u)
				sides = 6u;

			clear();

			// Set cofm to zero and recompute for this prism
			cofm = Vector3::zero();

			// Arrays of indices collected during construction of side walls to be used for end cap face creation.
			size_t* baseVertIndices = new size_t[sides];

			vertices.reserve(sides + 1u);
			faces.reserve(sides + 1u);
			edges.reserve(2u * sides);

			// Start creation of prism
			float alpha = 180.0f / (float)sides;
			Vector2 startPoint = Math::polygonStartingPoint(sides, size.x);

			float xStart = startPoint.x;
			float zStart = startPoint.y;
			G3D::Vector3 Sa(xStart, 0.0f, zStart);
			G3D::Vector3 Sb(0.0f, 0.0f, 0.0f);
			G3D::Vector3 axis(0.0f, 1.0f, 0.0f);

			// One vertex at apex
			// Vertex 0
			addVertex(0.0f, 0.5f * (float)size.y, 0.0f);

			// start at bottom corner vertex for first side wall
			// Vertex 1
			addVertex(xStart, -0.5f * (float)size.y, zStart);
			baseVertIndices[0] = 1u;

			// start accumulating cofm info.
			cofm += Sa;

			// Walk perimeter until last closing section is reached
			for (size_t i = 0u; i < sides - 1u; ++i) {
				G3D::Vector3 baseNorm = G3D::normalize(axis.cross(Sa));
				G3D::Vector3 SbRevNorm = G3D::normalize(-Sa);
				float effRad = Sa.magnitude();
				double sideLen = 2.0 * effRad * sin(0.0174533 * alpha);

				// update Sa to next facet
				Sb = Sa + sideLen * sin(0.0174533 * alpha) * SbRevNorm + sideLen * cos(0.0174533 * alpha) * baseNorm;
				cofm += Sb;
				Sa = Sb;

				addVertex(Sb.x, -0.5f * (float)size.y, Sb.z);
				baseVertIndices[i + 1u] = i + 2u;

				addFace(i + 1u, i + 2u, 0u);
			}

			// Finish off last side face
			addFace(sides, 1u, 0u);
			cofm.x = cofm.z = 0.0f;
			cofm.y = -float(size.y) / 6.0f;

			// Now the base
			addFace(sides, baseVertIndices, true);

			delete[] baseVertIndices;
		}

		void Mesh::makeParallelRamp(const Vector3& size, Vector3& cofm) {
			float x = 0.5f * size.x;
			float y = 0.5f * size.y;
			float z = 0.5f * size.z;

			//float connectionThickness = RBX::PartInstance::brickHeight();
			// Move away from 1.2 form factor
			float connectionThickness = 1.0f;

			clear();

			// important - don't reallocate while we are building
			vertices.reserve(8u);
			faces.reserve(6u);
			edges.reserve(12u);

			addVertex(x, -y + connectionThickness, z);
			addVertex(x, -y + connectionThickness, -z);
			addVertex(x, -y, z);
			addVertex(x, -y, -z);
			addVertex(-x, y, z);
			addVertex(-x, y, -z);
			addVertex(-x, y - connectionThickness, z);
			addVertex(-x, y - connectionThickness, -z);

			addFace(1u, 0u, 2u, 3u); // Legacy norm: x
			addFace(1u, 5u, 4u, 0u); // Legacy norm: y
			addFace(0u, 4u, 6u, 2u); // Legacy norm: z
			addFace(4u, 5u, 7u, 6u); // Legacy norm: -x
			addFace(7u, 3u, 2u, 6u); // Legacy norm: -y
			addFace(5u, 1u, 3u, 7u); // Legacy norm: -z
		}

		void Mesh::makeRightAngleRamp(const Vector3& size, Vector3& cofm) {
			float x = 0.5f * size.x;
			float y = 0.5f * size.y;
			float z = 0.5f * size.z;

			//float connectionThickness = RBX::PartInstance::brickHeight();
			// Move away from 1.2 form factor
			float connectionThickness = 1.0f;

			clear();

			// important - don't reallocate while we are building
			vertices.reserve(8u);
			faces.reserve(6u);
			edges.reserve(12u);

			addVertex(x, -y, z);
			addVertex(x, -y, -z);
			addVertex(x - connectionThickness, -y, z);
			addVertex(x - connectionThickness, -y, -z);
			addVertex(-x, y, z);
			addVertex(-x, y, -z);
			addVertex(-x, y - connectionThickness, z);
			addVertex(-x, y - connectionThickness, -z);

			addFace(1u, 0u, 2u, 3u); // Legacy norm: x
			addFace(1u, 5u, 4u, 0u); // Legacy norm: y
			addFace(0u, 4u, 6u, 2u); // Legacy norm: z
			addFace(4u, 5u, 7u, 6u); // Legacy norm: -x
			addFace(7u, 3u, 2u, 6u); // Legacy norm: -y
			addFace(5u, 1u, 3u, 7u); // Legacy norm: -z
		}

		void Mesh::makeCornerWedge(const Vector3& size, Vector3& cofm) {
			float x = 0.5f * size.x;
			float y = 0.5f * size.y;
			float z = 0.5f * size.z;

			clear();

			// important - don't reallocate while we are building
			vertices.reserve(5u);
			faces.reserve(5u);
			edges.reserve(8u);

			addVertex( x, -y,  z);
			addVertex( x, -y, -z);
			addVertex(-x, -y, -z);
			addVertex(-x, -y,  z);
			addVertex( x,  y, -z);

			addFace(0u, 1u, 4u);	 // Legacy norm: x
			addFace(0u, 4u, 3u);	 // Legacy norm: z
			addFace(2u, 3u, 4u);	 // Legacy norm: -x
			addFace(1u, 0u, 3u, 2u); // Legacy norm: -y
			addFace(1u, 2u, 4u);	 // Legacy norm: -z

			// calculate center-of-mass here: for a corner-wedge should lie 1/4 of the way up from the base (height is -y/2)
			//     and in the center of the rectangular cross-section there
			cofm = Vector3(x / 4.0f, -y / 2.0f, -z / 4.0f);
		}

		void Mesh::clear() {
			vertices.clear();
			edges.clear();
			faces.clear();
		}

		void Mesh::addVertex(float x, float y, float z) {
			size_t id = vertices.size();

			vertices.push_back(Vertex(id, Vector3(x, y, z)));
		}

		void Mesh::addFace(size_t i, size_t j, size_t k) {
			Edge* e0 = findOrMakeEdge(i, j);
			Edge* e1 = findOrMakeEdge(j, k);
			Edge* e2 = findOrMakeEdge(k, i);

			size_t id = faces.size();

			faces.push_back(Face(id, e0, e1, e2));

			Face* face = &faces[id];

			e0->addFace(face);
			e1->addFace(face);
			e2->addFace(face);

			face->initPlane();
		}

		void Mesh::addFace(size_t i, size_t j, size_t k, size_t l) {
			Edge* e0 = findOrMakeEdge(i, j);
			Edge* e1 = findOrMakeEdge(j, k);
			Edge* e2 = findOrMakeEdge(k, l);
			Edge* e3 = findOrMakeEdge(l, i);

			size_t id = faces.size();

			faces.push_back(Face(id, e0, e1, e2, e3));

			Face* face = &faces[id];

			e0->addFace(face);
			e1->addFace(face);
			e2->addFace(face);
			e3->addFace(face);

			face->initPlane();
		}

		void Mesh::addFace(size_t numVerts, size_t vertIndexList[], bool reverseOrder = false) {
			size_t id = faces.size();
			Edge* anEdge = nullptr;
			std::vector<Edge*> faceEdges;

			// find or make the edges and add them to a faceEdge container
			if (!reverseOrder) {
				for (size_t i = 0u; i < numVerts - 1u; i++) {
					anEdge = findOrMakeEdge(vertIndexList[i], vertIndexList[i + 1u]);
					if (anEdge)
						faceEdges.push_back(anEdge);
				}

				anEdge = findOrMakeEdge(vertIndexList[numVerts - 1u], vertIndexList[0]);

				if (anEdge)
					faceEdges.push_back(anEdge);
			}
			else {
				for (size_t i = 0u; i < numVerts - 1u; i++) {
					anEdge = findOrMakeEdge(vertIndexList[numVerts - 1u - i], vertIndexList[numVerts - 2u - i]);
					if (anEdge)
						faceEdges.push_back(anEdge);
				}

				anEdge = findOrMakeEdge(vertIndexList[0], vertIndexList[numVerts - 1u]);

				if (anEdge)
					faceEdges.push_back(anEdge);
			}


			// Create a face based on this edge list and add it to the face container
			faces.push_back(Face(id, faceEdges));
			Face* face = &faces[id];

			// Set each edge's face
			if (!reverseOrder) {
				for (size_t i = 0u; i < numVerts; i++) {
					anEdge = faceEdges[i];
					faceEdges.at(i)->addFace(face);
				}
			}
			else {
				for (size_t i = 0u; i < numVerts; i++) {
					anEdge = faceEdges[numVerts - 1u - i];
					faceEdges.at(numVerts - 1u - i)->addFace(face);
				}
			}


			face->initPlane();

		}

		Edge* Mesh::findOrMakeEdge(size_t v0, size_t v1) {
			Vertex* vert0 = &vertices[v0];
			Vertex* vert1 = &vertices[v1];

			if (Edge* found = vert0->findEdge(vert1)) {
				RBXASSERT(found->getVertex(nullptr, 0u) == vert1); // should be backwards - this is second face
				RBXASSERT(found->getVertex(nullptr, 1u) == vert0);
				RBXASSERT(found->getForward());
				RBXASSERT(!found->getBackward());

				return found;
			}
			else {
				return addEdge(vert0, vert1);
			}
		}

		Edge* Mesh::addEdge(Vertex* vert0, Vertex* vert1) {
			size_t id = edges.size();
			Edge edge(id, vert0, vert1);

			edges.push_back(edge);

			Edge* answer = &edges[id];

			vert0->addEdge(answer);
			vert1->addEdge(answer);

			return answer;
		}

		bool Mesh::pointInMesh(const Vector3& point) const {
			for (size_t i = 0u; i < numFaces(); ++i) {
				if (!getFace(i)->plane().pointOnOrBehind(point))
					return false;
			}

			return true;
		}


		const Face* Mesh::findFaceIntersection(const Vector3& inside, const Vector3& outside) const {
			RBXASSERT(pointInMesh(inside));
			RBXASSERT(!pointInMesh(outside));

			RbxRay ray = RbxRay::fromOriginAndDirection(inside, (outside - inside).direction());

			Vector3 tempHitPoint;

			for (size_t i = 0u; i < numFaces(); ++i) {
				const Face* face = getFace(i);

				if (rayIntersectsFace(ray, face, tempHitPoint)) {
					return face;
				}
			}

			RBXASSERT(0u);

			return nullptr;
		}

		void Mesh::findFaceIntersections(const Vector3& p0, const Vector3& p1, const Face*& f0, const Face*& f1) const {
			RBXASSERT(!pointInMesh(p0));
			RBXASSERT(!pointInMesh(p1));

			f0 = nullptr;
			f1 = nullptr;

			Line line = Line::fromTwoPoints(p0, p1);

			for (size_t i = 0u; i < numFaces(); ++i) {
				const Face* face = getFace(i);

				if (lineIntersectsFace(line, face)) {
					if (!f0)
						f0 = face;
					else
						f1 = face;

					return;
				}
			}
		}

		bool Mesh::rayIntersectsFace(const RbxRay& ray, const Face* face, Vector3& intersection) const {
			const Plane& plane = face->plane();
			intersection = ray.intersectionPlane(plane);

			if (intersection == Vector3::inf())
				return false;
			else {
				if (face->pointInFaceBorders(intersection))
					return true;
				else {
					intersection = Vector3::inf();
					return false;
				}
			}
		}

		bool Mesh::lineIntersectsFace(const Line& line, const Face* face) const {
			Vector3 point = line.intersection(face->plane());

			if (point == Vector3::inf())
				return false;
			else
				return face->pointInFaceBorders(point);
		}



		const Vertex* Mesh::farthestVertex(const Vector3& direction) const {
			const Vertex* answer = nullptr;
			float farthestDistance = -FLT_MAX;

			for (size_t i = 0u; i < numVertices(); ++i) {
				const Vertex* vertex = getVertex(i);
				float dot = direction.dot(vertex->getOffset());

				if (dot > farthestDistance) {
					farthestDistance = dot;
					answer = vertex;
				}
			}
			return answer;
		}

		bool Mesh::hitTest(const RbxRay& ray, Vector3& hitPoint, Vector3& surfaceNormal) const {
			float distance = FLT_MAX;
			size_t intersects = 0u;

			for (size_t i = 0u; i < numFaces(); ++i) {
				const Face* face = getFace(i);
				Vector3 tempHit;

				if (rayIntersectsFace(ray, face, tempHit)) {
					intersects++;
					float tempDistance = (tempHit - ray.origin()).squaredMagnitude();

					RBXASSERT(tempDistance >= 0.0f);

					if (tempDistance < distance) {
						hitPoint = tempHit;
						distance = tempDistance;
						surfaceNormal = face->normal();
					}
				}
			}

			return (intersects > 0u);
		}


		//////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////

		void Face::initPlane() {
			outwardPlane = Plane(getVertexOffset(0u), getVertexOffset(1u), getVertexOffset(2u));
		}

		const Face* Vertex::getFace(size_t i) const {
			return edges[i]->getVertexFace(this);
		}



		Face::Face(size_t id, Edge* e0, Edge* e1, Edge* e2)
			: id(id)
		{
			edges.push_back(e0);
			edges.push_back(e1);
			edges.push_back(e2);
		}

		Face::Face(size_t id, Edge* e0, Edge* e1, Edge* e2, Edge* e3)
			: id(id)
		{
			edges.push_back(e0);
			edges.push_back(e1);
			edges.push_back(e2);
			edges.push_back(e3);
		}

		Face::Face(size_t id, std::vector<Edge*>& edgeList)
			: id(id)
		{
			for (size_t i = 0u; i < edgeList.size(); i++)
				edges.push_back(edgeList[i]);
		}

		// TODO _ turn back on after identifying bug here
#pragma optimize( "", off )
		bool Face::pointInFaceBorders(const Vector3& point) const {
			for (size_t i = 0u; i < numEdges(); ++i) {
				const Edge* edge = getEdge(i);
				const Face* sideFace = edge->otherFace(this);

				if (!sideFace->plane().pointOnOrBehind(point))
					return false;
			}

			return true;
		}
#pragma optimize( "", on )

		int32_t Face::findInternalExtrusionIntersection(const Vector3& p0, const Vector3& p1) const {
			for (size_t i = 0u; i < numEdges(); ++i) {
				if (lineCrossesExtrusionSideBelow(p0, p1, i))
					return (int32_t)i;
			}

			return -1;
		}

		int32_t Face::getInternalExtrusionIntersection(const Vector3& pBelowInside, const Vector3& pBelowOutside) const {
			RBXASSERT(pointInInternalExtrusion(pBelowInside));
			RBXASSERT(!pointInInternalExtrusion(pBelowOutside));

			for (size_t i = 0u; i < numEdges(); ++i) {
				if (lineCrossesExtrusionSide(pBelowInside, pBelowOutside, i))
					return (int32_t)i;
			}

			return -1;
		}


		void Face::findInternalExtrusionIntersections(const Vector3& p0, const Vector3& p1, int32_t& side0, int32_t& side1) const {
			RBXASSERT(!pointInInternalExtrusion(p0));
			RBXASSERT(!pointInInternalExtrusion(p1));

			side0 = -1;
			side1 = -1;
			size_t found = 0u;

			for (size_t i = 0u; i < numEdges(); ++i) {
				if (lineCrossesExtrusionSideBelow(p0, p1, i)) {
					found++;

					if (side0 == -1)
						side0 = (int32_t)i;
					else
						side1 = (int32_t)i;
				}
			}
		}


		bool Face::lineCrossesExtrusionSideBelow(const Vector3& p0, const Vector3& p1, size_t edgeId) const {
			Plane sidePlane = getSidePlane(edgeId);

			if (sidePlane.halfSpaceContains(p0) != sidePlane.halfSpaceContains(p1)) { // points must be on opposite sides!
				Line line = Line::fromTwoPoints(p0, p1);

				Vector3 pointOnSide = line.intersection(sidePlane);

				if (plane().pointOnOrBehind(pointOnSide))
					return pointInExtrusionSide(pointOnSide, sidePlane, edgeId);
			}

			return false;
		}


		bool Face::lineCrossesExtrusionSide(const Vector3& p0, const Vector3& p1, size_t edgeId) const {
			Plane sidePlane = getSidePlane(edgeId);

			if (sidePlane.halfSpaceContains(p0) != sidePlane.halfSpaceContains(p1)) { // points must be on opposite sides!
				Line line = Line::fromTwoPoints(p0, p1);

				Vector3 pointOnSide = line.intersection(sidePlane);

				return pointInExtrusionSide(pointOnSide, sidePlane, edgeId);
			}

			return false;
		}

		bool Face::pointInExtrusionSide(const Vector3& pointOnSide, const Plane& sidePlane, size_t edgeId) const {
			Vector3 sidePlaneV = plane().normal().cross(sidePlane.normal());

			Plane sidePlane0 = Plane(sidePlaneV, getEdge(edgeId)->getVertexOffset(this, 0));

			if (sidePlane0.halfSpaceContains(pointOnSide)) {
				Plane sidePlane1 = Plane(-sidePlaneV, getEdge(edgeId)->getVertexOffset(this, 1));

				if (sidePlane1.halfSpaceContains(pointOnSide))
					return true;
			}

			return false;
		}

		Vector3 Face::getCentroid(void) const {
			Vector3 centroid = Vector3::zero();

			for (size_t i = 0u; i < numVertices(); ++i)
				centroid += getVertex(i)->getOffset();

			centroid /= (float)numVertices();

			return centroid;
		}

		void Face::getOrientedBoundingBox(const Vector3& xDir, const Vector3& yDir, Vector3& boxMin, Vector3& boxMax, Vector3& boxCenter) const {
			// This function finds a bounding box that is oriented with the x and y vector passed in.  It computes the min corner,
			// max corner, and the center of this box, in body coords.
			size_t numVerts = numVertices();
			Vector3 centroid = getCentroid();
			boxMin = Vector3::zero();
			boxMax = Vector3::zero();
			float xMax = -1e10f;
			float yMax = -1e10f;
			float xMin = 1e10f;
			float yMin = 1e10f;

			for (size_t i = 0u; i < numVerts; i++) {
				// max vector
				if ((getVertex(i)->getOffset() - centroid).dot(xDir) > xMax)
					xMax = (getVertex(i)->getOffset() - centroid).dot(xDir);
				if ((getVertex(i)->getOffset() - centroid).dot(yDir) > yMax)
					yMax = (getVertex(i)->getOffset() - centroid).dot(yDir);
				boxMax = centroid + xDir * xMax + yDir * yMax;

				// Min vector
				if ((getVertex(i)->getOffset() - centroid).dot(xDir) < xMin)
					xMin = (getVertex(i)->getOffset() - centroid).dot(xDir);
				if ((getVertex(i)->getOffset() - centroid).dot(yDir) < yMin)
					yMin = (getVertex(i)->getOffset() - centroid).dot(yDir);
				boxMin = centroid + xDir * xMin + yDir * yMin;
			}

			boxCenter = (boxMax + boxMin) * 0.5f;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		Edge* Vertex::findEdge(const Vertex* other) {
			for (size_t i = 0u; i < edges.size(); ++i) {
				Edge* e = edges[i];

				RBXASSERT(e->contains(this));

				if (e->contains(other))
					return e;
			}

			return nullptr;
		}

		const Edge* Vertex::recoverEdge(const Vertex* v0, const Vertex* v1) {
			for (size_t i = 0u; i < v0->numEdges(); ++i) {
				Edge* edge = v0->getEdge(i);
				if (edge->contains(v1))
					return edge;
			}

			RBXASSERT(0u);

			return nullptr;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		bool Edge::pointInVaronoi(const Vector3& point) const {
			const Vector3& v0 = getVertexOffset(nullptr, 0u);
			const Vector3& v1 = getVertexOffset(nullptr, 1u);

			Vector3 v0v1 = v1 - v0;
			Vector3 v0p = point - v0;
			Vector3 v1p = point - v1;

			if (v0v1.dot(v0p) < 0.0f)
				return false;
			else if (v0v1.dot(v1p) > 0.0f)
				return false;

			return true;
		}

	} // namespace POLY

} // namespace RBX
