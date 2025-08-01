/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/BulletShapeContact.h"
#include "V8Kernel/BulletShapeConnectors.h"
#include "V8Kernel/Kernel.h"
#include "V8World/Primitive.h"
#include "V8World/World.h"

#include "BulletCollision/CollisionDispatch/btCollisionObjectWrapper.h"
#include "btBulletCollisionCommon.h"
#include "BulletCollision/CollisionShapes/btConvexInternalShape.h"


namespace RBX {

	BulletShapeContact::BulletShapeContact(Primitive* p0, Primitive* p1, World* contactWorld) : Contact(p0, p1)
		, bulletManifold(nullptr)
		, bulletNPAlgorithm(nullptr)
		, world(contactWorld)
	{
	}

	BulletShapeContact::~BulletShapeContact() {
		deleteConnectors(polyConnectors);

		if (bulletNPAlgorithm) {
			bulletNPAlgorithm->~btCollisionAlgorithm();
			world->getBulletCollisionDispatcher()->freeCollisionAlgorithm(bulletNPAlgorithm);
		}

		RBXASSERT(polyConnectors.size() == 0u);
	}

	ContactConnector* BulletShapeContact::getConnector(size_t i) {
		return polyConnectors[i];
	}

	void BulletShapeContact::deleteAllConnectors() {
		deleteConnectors(polyConnectors);
	}


	void BulletShapeContact::deleteConnectors(BulletConnectorArray& deleteConnectors) {
		removeAllConnectorsFromKernel();

		for (size_t i = 0u; i < deleteConnectors.size(); ++i) {
			RBXASSERT(!deleteConnectors[i]->isInKernel());
			delete deleteConnectors[i];
		}

		deleteConnectors.fastClear();
	}


	void BulletShapeContact::removeAllConnectorsFromKernel() {
		Kernel* kernel = nullptr;

		for (size_t i = 0u; i < polyConnectors.size(); ++i) {
			if (polyConnectors[i]->isInKernel()) {
				kernel = kernel ? kernel : getKernel();		// small optimization - getKernel walks the IPipelines
				kernel->removeConnector(polyConnectors[i]);
			}
		}
	}

	void BulletShapeContact::putAllConnectorsInKernel() {
		Kernel* kernel = nullptr;

		for (size_t i = 0u; i < polyConnectors.size(); ++i) {
			//if (!polyConnectors[i]->isInKernel()  &&
			//	 polyConnectors[i]->getContactPoint().length < -ContactConnector::overlapGoal()) {
			if (!polyConnectors[i]->isInKernel()) {
				kernel = kernel ? kernel : getKernel();		// small optimization - getKernel walks the IPipelines
				kernel->insertConnector(polyConnectors[i]);
			}
		}
	}

	bool BulletShapeContact::stepContact() {
		if (computeIsColliding(0.0f)) {
			if (inKernel())
				putAllConnectorsInKernel();

			return true;
		}
		else {
			removeAllConnectorsFromKernel();

			return false;
		}
	}

	void BulletShapeContact::invalidateContactCache() {
		deleteAllConnectors();

		if (bulletNPAlgorithm) {
			btManifoldArray manifoldArray;
			bulletNPAlgorithm->getAllContactManifolds(manifoldArray);

			for (size_t i = 0u; i < manifoldArray.size(); ++i)
				manifoldArray[i]->clearManifold();
		}
	}

	bool BulletShapeContact::computeIsColliding(float overlapIgnored) {
		updateClosestFeatures();

		if (polyConnectors.size() > 0u) {
			float overlap = worstFeatureOverlap();

			if (overlap > overlapIgnored)
				return true;
		}

		return false;
	}

	void BulletShapeContact::updateClosestFeatures() {
		BulletConnectorArray newConnectors;

		findClosestFeatures(newConnectors);

		matchClosestFeatures(newConnectors);

		deleteConnectors(polyConnectors);		// any remaining not matched

		// set bullet's manifold point references back to roblox kernel connectors
		for (size_t i = 0u; i < newConnectors.size(); ++i) {
			btManifoldArray manifoldArray;

			bulletNPAlgorithm->getAllContactManifolds(manifoldArray);

			size_t manIndex = newConnectors[i]->getBulletManifoldIndex();
			size_t ptIndex = newConnectors[i]->getBulletPointCacheIndex();

			manifoldArray[manIndex]->getContactPoint(ptIndex).m_userPersistentData = newConnectors[i];
		}

		polyConnectors = newConnectors;			// transfer over the pointers

		updateContactPoints();
	}

	float BulletShapeContact::worstFeatureOverlap() {
		float worstOverlap = -FLT_MAX;		// i.e. not overlapping

		RBXASSERT(polyConnectors.size() > 0u);

		for (size_t i = 0u; i < polyConnectors.size(); ++i) {				// may not have any overlapping features!
			float overlap = polyConnectors[i]->computeOverlap();				// computeLength returns negative

			worstOverlap = std::max(worstOverlap, overlap);
		}

		return worstOverlap;
	}

	void BulletShapeContact::matchClosestFeatures(BulletConnectorArray& newConnectors) {
		for (size_t i = 0u; i < newConnectors.size(); ++i) {
			if (BulletShapeConnector* match = matchClosestFeature(newConnectors[i])) {
				delete newConnectors[i];

				newConnectors.replace(i, match);
			}
		}
	}

	BulletShapeConnector* BulletShapeContact::matchClosestFeature(BulletShapeConnector* newConnector) {
		for (size_t i = 0u; i < polyConnectors.size(); ++i) {
			BulletShapeConnector* answer = polyConnectors[i];

			if (BulletShapeConnector::match(answer, newConnector)) {
				polyConnectors.fastRemove(i);

				return answer;
			}
		}

		return nullptr;
	}

	void BulletShapeContact::updateContactPoints() {
		for (size_t i = 0u; i < polyConnectors.size(); ++i)
			polyConnectors[i]->findValidContactAfterNarrowphase();
	}

	BulletShapeConnector* BulletShapeContact::newBulletShapeConnector(btCollisionObject* bulletColObj0, btCollisionObject* bulletColObj1, btCollisionAlgorithm* algo, size_t manifoldIndex, size_t contactIndex, bool swapped = false) {
		if (contactParams) {
			return new BulletShapeConnector(getPrimitive(0u)->getBody(),
				getPrimitive(1u)->getBody(),
				*contactParams,
				bulletColObj0,
				bulletColObj1,
				algo,
				manifoldIndex,
				contactIndex
			); // id of this face
		}
		else
			return nullptr;
	}

	void BulletShapeContact::findClosestFeatures(BulletConnectorArray& newConnectors) {
		if (!contactParams)
			generateDataForMovingAssemblyStage();

		// Early out if CFrame is invalid
		if (Math::isNanInfVector3(getPrimitive(0)->getCoordinateFrame().translation) || Math::isNanInfVector3(getPrimitive(1)->getCoordinateFrame().translation))
			return;

		getPrimitive(0u)->updateBulletCollisionObject();
		getPrimitive(1u)->updateBulletCollisionObject();

		btManifoldArray manifoldArray;
		computeManifoldsWithBulletNarrowPhase(manifoldArray);

		size_t totalNumConnectors = 0u;
		for (size_t i = 0u; i < manifoldArray.size(); i++) {
			for (size_t j = 0u; j < manifoldArray[i]->getNumContacts(); j++) {
				bool valid = manifoldArray[i]->validContactDistance(manifoldArray[i]->getContactPoint(j));
				float dis = manifoldArray[i]->getContactPoint(j).getDistance();

				if (valid && dis < 0.0f) {
					RBXASSERT(totalNumConnectors < BULLET_CONTACT_ARRAY_SIZE);

					if (totalNumConnectors < BULLET_CONTACT_ARRAY_SIZE) {
						BulletShapeConnector* cylConn = newBulletShapeConnector(getPrimitive(0u)->getGeometry()->getBulletCollisionObject(),
							getPrimitive(1u)->getGeometry()->getBulletCollisionObject(),
							bulletNPAlgorithm, i, j);

						if (cylConn)
							newConnectors.push_back(cylConn);
					}
				}
			}
		}
	}

	void BulletShapeContact::computeManifoldsWithBulletNarrowPhase(btManifoldArray& manifoldArray) {
		btCollisionDispatcher* dispatcher;
		dispatcher = world->getBulletCollisionDispatcher();

		btCollisionObject* colObj0 = getPrimitive(0u)->getGeometry()->getBulletCollisionObject();
		btCollisionObject* colObj1 = getPrimitive(1u)->getGeometry()->getBulletCollisionObject();

		btCollisionObjectWrapper obj0Wrap(0, colObj0->getCollisionShape(), colObj0, colObj0->getWorldTransform(), -1, -1);
		btCollisionObjectWrapper obj1Wrap(0, colObj1->getCollisionShape(), colObj1, colObj1->getWorldTransform(), -1, -1);

		if (!bulletNPAlgorithm)
			bulletNPAlgorithm = dispatcher->findAlgorithm(&obj0Wrap, &obj1Wrap);

		btManifoldResult contactPointResult(&obj0Wrap, &obj1Wrap);
		btDispatcherInfo disInfo;
		bulletNPAlgorithm->processCollision(&obj0Wrap, &obj1Wrap, disInfo, &contactPointResult);
		bulletNPAlgorithm->getAllContactManifolds(manifoldArray);

		if (colObj0->getCollisionShape()->getShapeType() == GIMPACT_SHAPE_PROXYTYPE || colObj1->getCollisionShape()->getShapeType() == GIMPACT_SHAPE_PROXYTYPE) {
			//GImpact collision algorithm should only yield 1 Manifold
			RBXASSERT(manifoldArray.size() <= 1u);

			if (manifoldArray.size())
				manifoldArray[0]->refreshContactPoints(colObj0->getWorldTransform(), colObj1->getWorldTransform());
		}
	}

} // namespace
