#include "stdafx.h"


#include "V8World/TriangleMesh.h"
#include "G3D/CollisionDetection.h"
#include "G3D/Sphere.h"
#include "Util/Units.h"
#include "Util/Math.h"
#include "Util/RobloxGoogleAnalytics.h"
#include "V8World/Tolerance.h"
#include "V8World/MegaClusterPoly.h"

#include "LinearMath/btConvexHull.h"
#include "BulletCollision/CollisionShapes/btConvexHullShape.h"
#include "Extras/GIMPACTUtils/btGImpactConvexDecompositionShape.h"
#include "Extras/ConvexDecomposition/ConvexBuilder.h"

FASTFLAG(CSGPhysicsLevelOfDetailEnabled)

namespace CSGPhys {
	static const char* blockDataKey = "BLOCK";
	static const uint32_t blockKeyLen = 5u;
};

namespace RBX {

	const std::string physicsKeyTag("CSGPHS");

	KDTreeMeshWrapper::KDTreeMeshWrapper(const std::string& str) {
		btVector3 scale;
		int32_t currentVersion;

		std::vector<CSGConvex> meshConvexes = TriangleMesh::getDecompConvexes(str, currentVersion, scale);

		for (auto& convex : meshConvexes) {
			size_t indexOffset = vertices.size();

			for (auto& v : convex.vertices) {
				btVector3 tv = convex.transform * v;

				vertices.push_back(Vector3(tv.x(), tv.y(), tv.z()));
			}

			for (auto& i : convex.indices)
				indices.push_back(indexOffset + i);
		}

		if (!vertices.empty() && !indices.empty())
			tree.build(&vertices[0], nullptr, vertices.size(), &indices[0], indices.size() / 3u);
	}

	KDTreeMeshWrapper::~KDTreeMeshWrapper()
	{
	}

	TriangleMesh::~TriangleMesh() {
		if (boundingBoxMesh)
			delete boundingBoxMesh;
	}

	void TriangleMesh::setStaticMeshData(const std::string& key, const std::string& data, const btVector3& scale) {
		if (!validateDataVersions(data, version))
			return;

		kdTreeMesh = KDTreeMeshPool::getToken(key, data);
		kdTreeScale = Vector3(scale.x(), scale.y(), scale.z());
	}

	static bool validateDataHeader(std::stringstream& streamOut) {
		// Read Header
		char fileId[6] = { 0 };
		streamOut.read(fileId, 6);

		if (strncmp(fileId, "CSGPHS", 6u) != 0) 
			return false;

		return true;
	}

	bool TriangleMesh::validateDataVersions(const std::string& data, int32_t& version) {
		std::stringstream stream(data);

		BOOST_STATIC_ASSERT(sizeof(version) == 4u);

		// Read Header
		int32_t currentVersion = PHYSICS_SERIAL_VERSION;

		if (FFlag::CSGPhysicsLevelOfDetailEnabled) {
			if (!validateDataHeader(stream)) {
				version = -1;

				return false;
			}
		}
		else {
			char fileId[6] = { 0 };

			stream.read(fileId, 6);

			if (strncmp(fileId, "CSGPHS", 6u) != 0) {
				// error
				version = -1;

				return false;
			}
		}

		stream.read(reinterpret_cast<char*>(&version), sizeof(int32_t));
		if (version != currentVersion)
			return false;

		return true;
	}

	bool TriangleMesh::validateIsBlockData(const std::string& data) {
		std::stringstream stream(data);
		if (!validateDataHeader(stream))
			return false;

		int32_t version = 0;
		stream.read(reinterpret_cast<char*>(&version), sizeof(int32_t));

		if (version != 0)
			return false;

		char blockKeyword[CSGPhys::blockKeyLen] = { 0 };

		stream.read(blockKeyword, CSGPhys::blockKeyLen);

		if (strncmp(blockKeyword, CSGPhys::blockDataKey, CSGPhys::blockKeyLen) == 0)
			return true;

		return false;
	}

	std::string TriangleMesh::generateDecompositionGeometry(const std::vector<btVector3>& vertices, const std::vector<size_t>& indices) {
		if (vertices.size() > 0u && indices.size() > 0u) {
			// Reset Physics version, otherwise process placeholder data will loop endlessly
			version = PHYSICS_SERIAL_VERSION;

			std::string decompStr = generateDecompositionData(indices.size() / 3u, &indices[0], vertices.size(), (btScalar*)&vertices[0].x());

			return decompStr;
		}

		return "";
	}

	Matrix3 TriangleMesh::getMomentHollow(float mass) const {
		Vector3 size = getSize();

		float area = 2.0f * (size.x * size.y + size.y * size.z + size.z * size.x);

		Vector3 I;

		for (size_t i = 0u; i < 3u; i++) {
			size_t j = (i + 1u) % 3u;
			size_t k = (i + 2u) % 3u;
			float x = size[i]; // main axis;
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

	bool TriangleMesh::hitTest(const RbxRay& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal) {
		if (!kdTreeMesh)
			return false;

		const float maxDistance = MC_SEARCH_RAY_MAX;

		Vector3 from = rayInMe.origin() / kdTreeScale;
		Vector3 to = (rayInMe.origin() + maxDistance * rayInMe.direction()) / kdTreeScale;

		KDTree::RayResult result;
		kdTreeMesh->getTree().queryRay(result, from, to);

		if (result.hasHit()) {
			surfaceNormal = result.tree->getTriangleNormal(result.triangle);
			localHitPoint = (from + result.fraction * (to - from)) * kdTreeScale;
			return true;
		}

		return false;
	}

	void TriangleMesh::setSize(const G3D::Vector3& _size) {
		Super::setSize(_size);
		//RBXASSERT(getSize() == _size);

		centerToCornerDistance = 0.5f * _size.magnitude();

		boundingBoxMesh->setSize(_size);
	}

	bool TriangleMesh::setCompoundMeshData(const std::string& key, const std::string& data, const btVector3& scale) {
		if (!validateDataVersions(data, version))
			return false;

		//Convert Mesh Scale to String and append it to str before passing it off to geometry pool
		std::stringstream scaleStream;
		size_t scaleStride = sizeof(btVector3);

		scaleStream.write(reinterpret_cast<const char*>(&scaleStride), sizeof(size_t));
		scaleStream.write(reinterpret_cast<const char*>(&scale), scaleStride);

		std::string scaleStr(scaleStream.str().c_str(), scaleStream.str().size());

		// look at geometry pool for the equivalent decomp data before creating a new one
		try {
			compound = BulletDecompPool::getToken(scaleStr + key, scaleStr + data);
		}
		catch (std::exception& e) {
			StandardOut::singleton()->printf(MESSAGE_ERROR, "Solid Model - failed to load physics data: %s", e.what());

			return false;
		}

		return true;
	}

	bool TriangleMesh::setUpBulletCollisionData(void) {
		if (compound) {
			bulletCollisionObject->setCollisionShape(const_cast<BulletDecompWrapper::ShapeType*>(compound->getCompound()));

			return true;
		}
		else
			return false;
	}

	// Update Physics Data
	void TriangleMesh::updateObjectScale(const std::string& decompKey, const std::string& decompStr, const G3D::Vector3& scale, const G3D::Vector3& meshScale) {
		if (decompStr != "") {
			setCompoundMeshData(decompKey, decompStr, btVector3(scale.x, scale.y, scale.z));

			if (meshScale.x > 0.0f && meshScale.y > 0.0f && meshScale.z > 0.0f) {
				setStaticMeshData(decompKey, decompStr, btVector3(meshScale.x, meshScale.y, meshScale.z));

				return;
			}

			setStaticMeshData(decompKey, decompStr, btVector3(scale.x, scale.y, scale.z));
		}
	}

	// Serialization/Deserialization
	std::string TriangleMesh::generateDecompositionData(size_t numTriangles, const size_t* triangleIndexBase, size_t numVertices, btScalar* vertexBase) {
		int32_t currentVersion;
		std::stringstream decompStream;
		BulletConvexDecomposition	convexDecomposition;
		//-----------------------------------
		// Bullet Convex Decomposition
		//-----------------------------------
		btAlignedObjectArray<float> vertices;
		btAlignedObjectArray<size_t> indices;
		vertices.reserve(numVertices * 3u);
		indices.reserve(numTriangles * 3u);

		for (size_t vi = 0u; vi < numVertices; vi++) {
			size_t index = vi * 4u;
			btVector3 vec;

			vec[0] = vertexBase[index];
			vec[1] = vertexBase[index + 1u];
			vec[2] = vertexBase[index + 2u];
			vertices.push_back(vec[0]);
			vertices.push_back(vec[1]);
			vertices.push_back(vec[2]);
		}

		for (size_t i = 0u; i < numTriangles; i++) {
			size_t index = i * 3u;
			size_t i0, i1, i2;

			i0 = triangleIndexBase[index];
			i1 = triangleIndexBase[index + 1u];
			i2 = triangleIndexBase[index + 2u];
			indices.push_back(i0);
			indices.push_back(i1);
			indices.push_back(i2);
		}

		size_t depth;
		size_t maxVerticesPerHull;
		float cpercent;
		float ppercent;
		float skinWidth = 0.0f;

		depth = 6u;
		cpercent = 5u;
		ppercent = 5u;
		maxVerticesPerHull = 32u;

		ConvexDecomposition::DecompDesc desc;
		desc.mVcount = numVertices;
		desc.mVertices = &vertices[0];
		desc.mTcount = numTriangles;
		desc.mIndices = &indices[0];
		desc.mDepth = depth;
		desc.mCpercent = cpercent;
		desc.mPpercent = ppercent;
		desc.mMaxVertices = maxVerticesPerHull;
		desc.mSkinWidth = skinWidth;

		desc.mCallback = &convexDecomposition;
		desc.mCallbackOnlyNeedsHull = true;

		ConvexBuilder cb(desc.mCallback);
		cb.process(desc);

		currentVersion = PHYSICS_SERIAL_VERSION;

		// Generate String Stream
		decompStream.write("CSGPHS", 6u);
		decompStream.write(reinterpret_cast<const char*>(&currentVersion), sizeof(currentVersion));

		// Add convex child data to stream
		convexDecomposition.addStreamChildren(decompStream);

		std::string decompData(decompStream.str().c_str(), decompStream.str().size());

		size_t emptyLength = physicsKeyTag.size() + sizeof(currentVersion);

		if (decompData.length() <= emptyLength) {
			throw std::runtime_error("Generated empty Decomposition");
		}

		return decompData;
	}

	std::string TriangleMesh::generateConvexHullData(size_t numTriangles, const size_t* triangleIndexBase, size_t numVertices, const btVector3* vertexBase) {
		int32_t currentVersion;
		std::stringstream decompStream;

		HullDesc hd;
		hd.mFlags = QF_TRIANGLES;
		hd.mVcount = numVertices;
		hd.mVertices = vertexBase;
		hd.mVertexStride = sizeof(btVector3);

		HullLibrary hl;
		HullResult hr;

		if (hl.CreateConvexHull(hd, hr) == QE_FAIL)
			throw std::runtime_error("Could not generate convex hull");
		else {
			currentVersion = PHYSICS_SERIAL_VERSION;

			// Generate String Stream
			decompStream.write("CSGPHS", 6u);
			decompStream.write(reinterpret_cast<const char*>(&currentVersion), sizeof(currentVersion));

			// Create Placeholder Translation
			btTransform trans;
			trans.setIdentity();

			btAlignedObjectArray<float>	vertFloats;
			vertFloats.reserve(hr.mNumOutputVertices * 3u);

			// Have to align here for serialization because btVector3 is 4-Dim
			for (size_t vi = 0u; vi < hr.mNumOutputVertices; vi++) {
				vertFloats.push_back(hr.m_OutputVertices[vi][0]);
				vertFloats.push_back(hr.m_OutputVertices[vi][1]);
				vertFloats.push_back(hr.m_OutputVertices[vi][2]);
			}

			size_t numVertexFloat = hr.mNumOutputVertices * 3u;
			size_t numIndices = hr.mNumIndices;
			serializeConvexHullData(trans, numVertexFloat, &vertFloats[0], numIndices, &hr.m_Indices[0], decompStream);

			std::string decompData(decompStream.str());

			size_t emptyLength = physicsKeyTag.size() + sizeof(currentVersion);

			if (decompData.length() <= emptyLength)
				throw std::runtime_error("Generated empty Decomposition");

			return decompData;
		}
	}

	BulletDecompWrapper::ShapeType* TriangleMesh::retrieveDecomposition(const std::string& str) {
		BulletDecompWrapper::ShapeType* decomp = new BulletDecompWrapper::ShapeType();

		btVector3 scale;
		int32_t currentVersion;

		std::vector<CSGConvex> meshConvexes = getDecompConvexes(str, currentVersion, scale, true);

		for (size_t i = 0u; i < meshConvexes.size(); i++) {
			btAlignedObjectArray<btVector3> vertices;
			btTransform trans;

			btCollisionShape* convexShape = new btConvexHullShape(&(meshConvexes[i].vertices[0].getX()), meshConvexes[i].vertices.size(), sizeof(btVector3));
			convexShape->setLocalScaling(scale);
			convexShape->setMargin(bulletCollisionMargin);

			decomp->addChildShape(meshConvexes[i].transform, convexShape);
		}

		// Generation got bad data
		if (decomp->getNumChildShapes() == 0) {
			delete decomp;
			throw std::runtime_error("Decomp data has no children");
		}

#ifdef USE_GIMPACT
		decomp->updateBound();
#endif

		return decomp;
	}

	std::string TriangleMesh::generateStaticMeshData(const std::vector<size_t>& indices, std::vector<btVector3>& vertices) {
		std::stringstream stream;
		size_t numVertices = vertices.size();
		size_t vertexStride = sizeof(btVector3);
		stream.write(reinterpret_cast<const char*>(&numVertices), sizeof(size_t));
		stream.write(reinterpret_cast<const char*>(&vertexStride), sizeof(size_t));
		stream.write(reinterpret_cast<const char*>(&vertices[0]), static_cast<std::streamsize>(vertexStride) * numVertices);

		size_t numIndices = indices.size();
		stream.write(reinterpret_cast<const char*>(&numIndices), sizeof(size_t));
		stream.write(reinterpret_cast<const char*>(&indices[0]), sizeof(size_t) * static_cast<std::streamsize>(numIndices));

		std::string buffer(stream.str().c_str(), stream.str().size());

		return buffer;
	}

	void TriangleMesh::readPrefixData(btVector3& scale, int& currentVersion, std::stringstream& stream) {
		// Read Scale
		size_t scaleStride = 0u;
		stream.read(reinterpret_cast<char*>(&scaleStride), sizeof(size_t));
		stream.read(reinterpret_cast<char*>(&scale), scaleStride);

		// Read Version
		char fileId[6] = { 0 };
		stream.read(fileId, 6u);
		stream.read(reinterpret_cast<char*>(&currentVersion), sizeof(int32_t));
	}

	void TriangleMesh::readConvexHullData(std::vector<float>& vertices, size_t& numVertices, std::vector<size_t>& indices, size_t& numIndices, btTransform& trans, std::stringstream& stream) {
		btVector3 transformTrans;
		btQuaternion transformRot;
		size_t translationStride = 0u;
		size_t rotationStride = 0u;

		// Read Transform
		stream.read(reinterpret_cast<char*>(&translationStride), sizeof(size_t));
		stream.read(reinterpret_cast<char*>(&transformTrans), translationStride);
		stream.read(reinterpret_cast<char*>(&rotationStride), sizeof(size_t));
		stream.read(reinterpret_cast<char*>(&transformRot), rotationStride);

		// Set Transform
		trans.setOrigin(transformTrans);
		trans.setRotation(transformRot);

		// Read Vertices
		size_t vertexStride = 0u;
		stream.read(reinterpret_cast<char*>(&numVertices), sizeof(size_t));
		stream.read(reinterpret_cast<char*>(&vertexStride), sizeof(size_t));
		vertices.resize(numVertices);
		stream.read(reinterpret_cast<char*>(&vertices[0]), static_cast<std::streamsize>(vertexStride) * numVertices);

		// Read Indices
		stream.read(reinterpret_cast<char*>(&numIndices), sizeof(size_t));
		indices.resize(numIndices);
		stream.read(reinterpret_cast<char*>(&indices[0]), static_cast<std::streamsize>(sizeof(size_t)) * numIndices);
	}

	void TriangleMesh::serializeConvexHullData(const btTransform& transform, const size_t numVertices, const float* verticesBase,
		const size_t numIndices, const size_t* indicesBase, std::stringstream& outstream)
	{
		btVector3 transformTrans = transform.getOrigin();
		btQuaternion transformRot = transform.getRotation();

		// serialization
		// - Translation 
		size_t translationStride = sizeof(btVector3);
		size_t rotationStride = sizeof(btQuaternion);
		outstream.write(reinterpret_cast<const char*>(&translationStride), sizeof(size_t));
		outstream.write(reinterpret_cast<const char*>(&transformTrans), translationStride);
		outstream.write(reinterpret_cast<const char*>(&rotationStride), sizeof(size_t));
		outstream.write(reinterpret_cast<const char*>(&transformRot), rotationStride);


		// - Vertices
		size_t vertexStride = sizeof(float);
		outstream.write(reinterpret_cast<const char*>(&numVertices), sizeof(size_t));
		outstream.write(reinterpret_cast<const char*>(&vertexStride), sizeof(size_t));
		outstream.write(reinterpret_cast<const char*>(verticesBase), static_cast<std::streamsize>(vertexStride) * numVertices);

		// - Indices
		outstream.write(reinterpret_cast<const char*>(&numIndices), sizeof(size_t));
		outstream.write(reinterpret_cast<const char*>(indicesBase), sizeof(size_t) * static_cast<std::streamsize>(numIndices));
	}

	std::vector<CSGConvex> TriangleMesh::getDecompConvexes(const std::string& data, int32_t& currentVersion, btVector3& scale, bool dataHasScale) {
		std::vector<CSGConvex> outputConvexes;
		std::stringstream stream(data);

		if (dataHasScale) {
			// Read Scale
			size_t scaleStride = 0u;
			stream.read(reinterpret_cast<char*>(&scaleStride), sizeof(size_t));
			stream.read(reinterpret_cast<char*>(&scale), scaleStride);
		}
		else {
			scale = btVector3(1, 1, 1);
		}

		// Read Version
		char fileId[6] = { 0 };
		stream.read(fileId, 6u);
		stream.read(reinterpret_cast<char*>(&currentVersion), sizeof(int32_t));

		while (stream.rdbuf()->in_avail() > 0) {
			CSGConvex currentConvex;
			btTransform trans;
			size_t numVertices = 0u;
			size_t numIndices = 0u;
			std::vector<float> hullVertices;
			btVector3 transformTrans;
			btQuaternion transformRot;
			size_t translationStride = 0u;
			size_t rotationStride = 0u;

			// Read Transform
			stream.read(reinterpret_cast<char*>(&translationStride), sizeof(size_t));
			stream.read(reinterpret_cast<char*>(&transformTrans), translationStride);
			stream.read(reinterpret_cast<char*>(&rotationStride), sizeof(size_t));
			stream.read(reinterpret_cast<char*>(&transformRot), rotationStride);

			// Set Transform
			trans.setOrigin(transformTrans);
			trans.setRotation(transformRot);

			// Read Vertices
			size_t vertexStride = 0u;
			stream.read(reinterpret_cast<char*>(&numVertices), sizeof(size_t));
			stream.read(reinterpret_cast<char*>(&vertexStride), sizeof(size_t));
			hullVertices.resize(numVertices);
			stream.read(reinterpret_cast<char*>(&hullVertices[0]), static_cast<std::streamsize>(vertexStride) * numVertices);

			// Read Indices
			stream.read(reinterpret_cast<char*>(&numIndices), sizeof(size_t));
			currentConvex.indices.resize(numIndices);
			stream.read(reinterpret_cast<char*>(&currentConvex.indices[0]), sizeof(size_t) * static_cast<std::streamsize>(numIndices));

			for (size_t i = 0u; i < (numVertices / 3u); i++) {
				btVector3 vertex(hullVertices[i * 3u], hullVertices[i * 3u + 1u], hullVertices[i * 3u + 2u]);

				currentConvex.vertices.push_back(vertex);
			}

			currentConvex.transform = trans;
			outputConvexes.push_back(currentConvex);
		}

		return outputConvexes;
	}

	void BulletConvexDecomposition::ConvexDecompResult(ConvexDecomposition::ConvexResult& result) {
		// Create Placeholder Translation
		btTransform trans;
		trans.setIdentity();

		if (FFlag::CSGPhysicsLevelOfDetailEnabled) {
			size_t numVertexFloats = result.mHullVcount * 3u;
			size_t numIndices = result.mHullTcount * 3u;
			TriangleMesh::serializeConvexHullData(trans, numVertexFloats, &result.mHullVertices[0], numIndices, &result.mHullIndices[0], streamChildren);
		}
		else {
			btVector3 transformTrans = trans.getOrigin();
			btQuaternion transformRot = trans.getRotation();

			// serialization
			// - Translation 
			size_t translationStride = sizeof(btVector3);
			size_t rotationStride = sizeof(btQuaternion);
			streamChildren.write(reinterpret_cast<const char*>(&translationStride), sizeof(size_t));
			streamChildren.write(reinterpret_cast<const char*>(&transformTrans), translationStride);
			streamChildren.write(reinterpret_cast<const char*>(&rotationStride), sizeof(size_t));
			streamChildren.write(reinterpret_cast<const char*>(&transformRot), rotationStride);


			// - Vertices
			size_t numVertices = result.mHullVcount * 3u;
			size_t vertexStride = sizeof(float);
			streamChildren.write(reinterpret_cast<const char*>(&numVertices), sizeof(size_t));
			streamChildren.write(reinterpret_cast<const char*>(&vertexStride), sizeof(size_t));
			streamChildren.write(reinterpret_cast<const char*>(&result.mHullVertices[0]), static_cast<std::streamsize>(vertexStride) * numVertices);

			// - Indices
			size_t numIndices = result.mHullTcount * 3u;
			streamChildren.write(reinterpret_cast<const char*>(&numIndices), sizeof(size_t));
			streamChildren.write(reinterpret_cast<const char*>(&result.mHullIndices[0]), sizeof(size_t) * static_cast<std::streamsize>(numIndices));
		}
	}

	const std::string TriangleMesh::getPlaceholderData() {
		std::stringstream decompStream;
		size_t nonDecompVersion = 0u;

		decompStream.write("CSGPHS", 6u);
		decompStream.write(reinterpret_cast<const char*>(&nonDecompVersion), sizeof(nonDecompVersion));

		std::string decompData(decompStream.str());

		return decompData;
	}

	const std::string TriangleMesh::getBlockData() {
		std::string result = getPlaceholderData();

		result.append(CSGPhys::blockDataKey);

		return result;
	}

	void BulletConvexDecomposition::addStreamChildren(std::stringstream& streamString) {
		streamString.write(streamChildren.str().c_str(), streamChildren.str().size());
	}

	size_t TriangleMesh::closestSurfaceToPoint(const Vector3& pointInBody) const {
		return boundingBoxMesh->closestSurfaceToPoint(pointInBody);
	}

	Plane TriangleMesh::getPlaneFromSurface(const size_t surfaceId) const {
		return boundingBoxMesh->getPlaneFromSurface(surfaceId);
	}

	Vector3 TriangleMesh::getSurfaceNormalInBody(const size_t surfaceId) const {
		return boundingBoxMesh->getSurfaceNormalInBody(surfaceId);
	}

	Vector3 TriangleMesh::getSurfaceVertInBody(const size_t surfaceId, const size_t vertId) const {
		return boundingBoxMesh->getSurfaceVertInBody(surfaceId, vertId);
	}

	size_t TriangleMesh::getMostAlignedSurface(const Vector3& vecInWorld, const G3D::Matrix3& objectR) const {
		return boundingBoxMesh->getMostAlignedSurface(vecInWorld, objectR);
	}

	size_t TriangleMesh::getNumVertsInSurface(const size_t surfaceId) const {
		return boundingBoxMesh->getNumVertsInSurface(surfaceId);
	}

	bool TriangleMesh::vertOverlapsFace(const Vector3& pointInBody, const size_t surfaceId) const {
		return boundingBoxMesh->vertOverlapsFace(pointInBody, surfaceId);
	}

	CoordinateFrame TriangleMesh::getSurfaceCoordInBody(const size_t surfaceId) const {
		return boundingBoxMesh->getSurfaceCoordInBody(surfaceId);
	}

	bool TriangleMesh::findTouchingSurfacesConvex(const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId) const {
		return boundingBoxMesh->findTouchingSurfacesConvex(myCf, myFaceId, otherGeom, otherCf, otherFaceId);
	}

	bool TriangleMesh::FacesOverlapped(const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol) const {
		return boundingBoxMesh->FacesOverlapped(myCf, myFaceId, otherGeom, otherCf, otherFaceId, tol);
	}

	bool TriangleMesh::FaceVerticesOverlapped(const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol) const {
		return boundingBoxMesh->FaceVerticesOverlapped(myCf, myFaceId, otherGeom, otherCf, otherFaceId, tol);
	}

	bool TriangleMesh::FaceEdgesOverlapped(const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol) const {
		return boundingBoxMesh->FaceEdgesOverlapped(myCf, myFaceId, otherGeom, otherCf, otherFaceId, tol);
	}

} // namespace RBX


// Randomized Locations for hackflags
namespace RBX {
	namespace Security {
		size_t hackFlag11 = 0u;
	};
};
