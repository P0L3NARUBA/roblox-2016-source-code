/* Copyright 2014 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "g3d/platform.h"
#include "g3d/g3dmath.h"

#include "G3D/Vector2.h"
#include "G3D/Vector3.h"
#include "G3D/Vector4.h"
#include "G3D/Color4.h"
#include "G3D/CoordinateFrame.h"

#include "rbx/Debug.h"


namespace RBX {

	class CSGVertex {
	public:
		G3D::Vector3 Position;
		G3D::Vector2 UV;
		G3D::Color4 Color;
		//G3D::Color4uint8 extra; // red = uv generation type
		//G3D::Vector2 uvStuds;
		//G3D::Vector2 UVDecal;

		G3D::Vector3 Normal;
		G3D::Vector3 Tangent;
		//G3D::Vector4 edgeDistances;

		enum UVGenerationType {
			NO_UV_GENERATION = 0,
			UV_BOX_X,
			UV_BOX_Y,
			UV_BOX_Z,
			UV_BOX_X_NEG,
			UV_BOX_Y_NEG,
			UV_BOX_Z_NEG
		};


		CSGVertex() { ; }

		G3D::Vector2 generateUv(const Vector3& pos) const;
		void generateUv();
	};

	class CSGMesh {
	public:
		CSGMesh();
		virtual ~CSGMesh();

		virtual CSGMesh* clone() const;

		const std::vector<CSGVertex>& getVertices() const { return vertices; }
		const std::vector<uint32_t>& getIndices() const { return indices; }

		const std::vector<uint32_t>& getIndexRemap(uint32_t idx) const { RBXASSERT(idx < 6u); return decalIndexRemap[idx]; }
		const std::vector<uint32_t>& getVertexRemap(uint32_t idx) const { RBXASSERT(idx < 6u); return decalVertexRemap[idx]; }

		std::string createHash(const std::string salt = "") const;

		bool isBadMesh() const { return badMesh; }
		virtual bool isValid() const { return true; }

		void set(const std::vector<CSGVertex>& vertices, const std::vector<uint32_t>& indices);
		void clearMesh();

		virtual void translate(const G3D::Vector3& translation) {}

		virtual void applyCoordinateFrame(G3D::CoordinateFrame cFrame) {}
		virtual void applyTranslation(const G3D::Vector3& trans) {}
		virtual void applyColor(const G3D::Vector3& color) {}
		virtual void applyScale(const G3D::Vector3& size) {}
		virtual void triangulate() {}
		virtual bool newTriangulate() { return true; }
		virtual void weldMesh(bool positionOnly = false) {}

		virtual void buildBRep() {}

		virtual bool unionMesh(const CSGMesh* a, const CSGMesh* b) { return false; }
		virtual bool intersectMesh(const CSGMesh* a, const CSGMesh* b) { return false; }
		virtual bool subractMesh(const CSGMesh* a, const CSGMesh* b) { return false; }

		bool isNotEmpty() const;

		std::string toBinaryString() const;
		std::string toBinaryStringForPhysics() const;
		bool fromBinaryString(const std::string& str);
		virtual std::string getBRepBinaryString() const { return ""; }
		virtual void setBRepFromBinaryString(const std::string& str) {}

		virtual size_t clusterVertices(float resolution) { return 0; }
		virtual bool makeHalfEdges(std::vector<int32_t>& vertexEdges) { return true; }

		virtual G3D::Vector3 extentsCenter() { G3D::Vector3 v; return v; }
		virtual G3D::Vector3 extentsSize() { G3D::Vector3 v; return v; }

		void computeDecalRemap();

	protected:
		uint32_t version;
		uint32_t brepVersion;
		bool badMesh;

		std::vector<CSGVertex> vertices;
		std::vector<uint32_t> indices;

		std::vector<uint32_t> decalVertexRemap[6];
		std::vector<uint32_t> decalIndexRemap[6];
	};

	class CSGMeshFactory {
	public:
		virtual CSGMesh* createMesh();

		static CSGMeshFactory* singleton();
		static void set(CSGMeshFactory* factory);
	};

}
