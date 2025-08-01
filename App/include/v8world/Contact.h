/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/Edge.h"
#include "V8World/Feature.h"
#include "V8Kernel/ContactParams.h"
#include "Util/G3DCore.h"
#include "Util/Memory.h"
#include "Util/NormalId.h"
#include "Util/Math.h"
#include "Util/FixedArray.h"
#include "rbx/Debug.h"
#include "rbx/Declarations.h"

#define CONTACT_ARRAY_SIZE 40
#define BULLET_CONTACT_ARRAY_SIZE 40

namespace RBX {

	class Kernel;
	class CollisionStage;
	class ContactConnector;
	class GeoPairConnector;
	class BallBlockConnector;
	class BallBallConnector;
	class Body;
	class Ball;
	class Block;
	class BlockBlockContactData;

	class RBXBaseClass Contact : public Edge {
	private:
		typedef Edge Super;

		friend class CollisionStage;


		static bool			ignoreBool;

		// CollisionStage
		int32_t	lastUiContactStep;
		int32_t	steppingIndex;			// For fast removal from the collision stage stepping list
		short	numTouchCycles;

		/////////////////////////////////////////////////////
		//
		// Edge

		/*override*/ void putInKernel(Kernel* _kernel) {
			Super::putInKernel(_kernel);
		}

		/*override*/ void removeFromKernel() {
			RBXASSERT(getKernel());
			deleteAllConnectors();
			Super::removeFromKernel();
		}

		///////////////////////////////////////////////////
		// Edge Virtuals
		//
		/*override*/ virtual EdgeType getEdgeType() const { return Edge::CONTACT; }

	protected:
		ContactParams* contactParams;
		Body* getBody(uint32_t i);

		/////////////////////////////////////////////////////
		//
		// ContactPairData management

		void deleteConnector(ContactConnector* c);

		virtual void deleteAllConnectors() = 0;		// everyone implements this

		virtual	bool stepContact() = 0;

	public:
		enum ContactType {
			Contact_Simple,
			Contact_Cell,
			Contact_Buoyancy,
		};

		Contact(Primitive* p0, Primitive* p1);

		virtual ~Contact();

		short getNumTouchCycles() const { return numTouchCycles; }

		int32_t& steppingIndexFunc() { return steppingIndex; }		// fast removal from stepping list

		// Proximite tests - compute
		typedef bool (Contact::* ProximityTest)(float);

		bool computeIsAdjacentUi(float spaceAllowed);

		virtual bool computeIsCollidingUi(float overlapIgnored);

		virtual bool computeIsColliding(float overlapIgnored) = 0;

		static bool isContact(Edge* e) { return (e->getEdgeType() == Edge::CONTACT); }

		/////////////////////////////////////////////////////
		//
		// From The Contact Manager
		virtual void onPrimitiveContactParametersChanged();

		bool step(int32_t uiStepId);

		virtual size_t numConnectors() const = 0;

		virtual ContactConnector* getConnector(uint32_t i) = 0;

		void primitiveMovedExternally();

		virtual void generateDataForMovingAssemblyStage(void);

		virtual void invalidateContactCache();

		ContactParams* getContactParams(void) { return contactParams; }

		bool isInContact() const { return lastUiContactStep > 0; }

		virtual ContactType getContactType() const { return Contact_Simple; }
	};

	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////

	class BallBallContact
		: public Contact
		, public Allocator<BallBallContact>
	{
	private:
		BallBallConnector* ballBallConnector;

		Ball* ball(uint32_t i);

		/*override*/ void deleteAllConnectors();
		/*override*/ bool computeIsColliding(float overlapIgnored);
		/*override*/ bool stepContact();

		/*override*/  size_t numConnectors() const { return ballBallConnector ? 1u : 0u; }
		/*override*/  ContactConnector* getConnector(uint32_t i);

	public:
		BallBallContact(Primitive* p0, Primitive* p1)
			: Contact(p0, p1)
			, ballBallConnector(nullptr)
		{
		}

		~BallBallContact() { RBXASSERT(!ballBallConnector); }

		void generateDataForMovingAssemblyStage(void); /*override*/
	};

	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	class BallBlockContact
		: public Contact
		, public Allocator<BallBlockContact>
	{
	private:
		BallBlockConnector* ballBlockConnector;

		Primitive* ballPrim();
		Primitive* blockPrim();

		Ball* ball();
		Block* block();

		bool computeIsColliding(
			uint32_t& onBorder,
			Vector3int16& clip,
			Vector3& projectionInBlock,
			float overlapIgnored);

		/*override*/ void deleteAllConnectors();
		/*override*/ bool computeIsColliding(float overlapIgnored);
		/*override*/ bool stepContact();

		/*override*/  size_t numConnectors() const { return ballBlockConnector ? 1u : 0u; }
		/*override*/  ContactConnector* getConnector(uint32_t i);

	public:
		BallBlockContact(Primitive* p0, Primitive* p1)
			: Contact(p0, p1)
			, ballBlockConnector(nullptr)
		{
		}

		~BallBlockContact() { RBXASSERT(!ballBlockConnector); }

		void generateDataForMovingAssemblyStage(void); /*override*/
	};

	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////

	class BlockBlockContact
		: public Contact
		, public Allocator<BlockBlockContact>
	{
	private:
		static uint32_t pairMatches;
		static uint32_t pairMisses;
		static uint32_t featureMatches;
		static uint32_t featureMisses;

		typedef RBX::FixedArray<GeoPairConnector*, 8> ConnectorArray;

		friend class BlockBlockContactData;
		BlockBlockContactData* myData;

		Block* block(uint32_t i);

		GeoPairConnector* findGeoPairConnector(
			Body* b0,
			Body* b1,
			GeoPairType _pairType,
			uint32_t param0,
			uint32_t param1);

		void loadGeoPairEdgeEdge(
			uint32_t b0,
			uint32_t b1,
			int32_t edge0,
			int32_t edge1);

		void loadGeoPairPointPlane(
			uint32_t pointBody,
			uint32_t planeBody,
			uint32_t pointID,
			NormalId pointFaceID,
			NormalId planeFaceID);

		// plane contact - first compute the feature0, feature1
		bool getBestPlaneEdge(float overlapIgnored, bool& planeContact);

		uint32_t intersectRectQuad(Vector2& planeRect, Vector2(&otherQuad)[4]);

		bool computeIsColliding(float overlapIgnored, bool& planeContact);

		////////////////////////////////////////////////////
		// Contact
		/*override*/ void deleteAllConnectors(void);
		/*override*/ bool computeIsColliding(float overlapIgnored);
		/*override*/ bool stepContact();

		/*override*/  size_t numConnectors() const;
		/*override*/  ContactConnector* getConnector(uint32_t i);

	public:
		BlockBlockContact(Primitive* p0, Primitive* p1);
		~BlockBlockContact();

		static float pairHitRatio();
		static float featureHitRatio();

		void generateDataForMovingAssemblyStage(void); /*override*/

	private:
		inline static void boxProjection(const Vector3& normal0, const Matrix3& R1, const Vector3& extent1, float& projectedExtent) {
			Vector3 temp = Math::vectorToObjectSpace(normal0, R1);
			projectedExtent = std::abs(extent1[0] * temp[0])
				+ std::abs(extent1[1] * temp[1])
				+ std::abs(extent1[2] * temp[2]);
		}

		// if length < 0, no overlap, and no block/block contact
		// proj0 >0 on entry
		// proj1 >0 on entry

		inline static bool updateBestAxis(float proj0, float p0p1, float proj1, float& _overlap, float overlapIgnored) {
			// no overlap along this axis - bail, not in contact
			_overlap = proj0 + proj1 - std::abs(p0p1);
			return (_overlap > overlapIgnored);
		}

		bool geoFeaturesOverlap(
			uint32_t pointBody,
			uint32_t planeBody,
			uint32_t pointID,
			NormalId pointFaceID,
			NormalId planeFaceID);
	};

	class BlockBlockContactData
	{
		friend class BlockBlockContact;

	private:
		BlockBlockContact::ConnectorArray connectors[2];
		uint32_t connectorsIndex;

		// for hysteresis:
		uint32_t		witnessId;
		uint32_t		separatingAxisId;

		int32_t			feature[2];	// -1 no feature, 0..5 plane, 6..8 edge Normal, 
		uint32_t		bPlane;
		uint32_t		bOther;
		RBX::NormalId	planeID;
		RBX::NormalId	otherPlaneID;

		BlockBlockContact* myOwner;

	public:
		BlockBlockContactData(BlockBlockContact* owner);
		~BlockBlockContactData() {}

		size_t numConnectors() const { return connectors[connectorsIndex].size(); }
		ContactConnector* getConnector(uint32_t i);
		void clearConnectors(void);
		GeoPairConnector* findGeoPairConnector(Body* b0, Body* b1, GeoPairType _pairType, uint32_t param0, uint32_t param1);
		bool stepContact();
		void loadGeoPairEdgeEdgePlane(uint32_t edgeBody, uint32_t planeBody, int32_t edge0, int32_t edge1);
		bool getBestPlaneEdge(float overlapIgnored, bool& planeContact);
		uint32_t computePlaneContact(void);
		uint32_t intersectRectQuad(Vector2& planeRect, Vector2(&otherQuad)[4]);
	};

} // namespace
