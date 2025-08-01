/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef BT_PERSISTENT_MANIFOLD_H
#define BT_PERSISTENT_MANIFOLD_H


#include "LinearMath/btVector3.h"
#include "LinearMath/btTransform.h"
#include "btManifoldPoint.h"
class btCollisionObject;
#include "LinearMath/btAlignedAllocator.h"

struct btCollisionResult;

///maximum contact breaking and merging threshold
extern btScalar gContactBreakingThreshold;

// ROBLOX HOOKS
extern btScalar gContactThresholdOrthogonalFactor;
extern btScalar gContactThresholdOrthogonalActivateFactor;

typedef bool (*ContactDestroyedCallback)(void* userPersistentData);
typedef bool (*ContactProcessedCallback)(btManifoldPoint& cp, void* body0, void* body1);
extern ContactDestroyedCallback	gContactDestroyedCallback;
extern ContactProcessedCallback gContactProcessedCallback;

//the enum starts at 1024 to avoid type conflicts with btTypedConstraint
enum btContactManifoldTypes {
	MIN_CONTACT_MANIFOLD_TYPE = 1024,
	BT_PERSISTENT_MANIFOLD_TYPE
};

#define MANIFOLD_CACHE_SIZE 4

///btPersistentManifold is a contact point cache, it stays persistent as long as objects are overlapping in the broadphase.
///Those contact points are created by the collision narrow phase.
///The cache can be empty, or hold 1,2,3 or 4 points. Some collision algorithms (GJK) might only add one point at a time.
///updates/refreshes old contact points, and throw them away if necessary (distance becomes too large)
///reduces the cache to 4 points, when more then 4 points are added, using following rules:
///the contact point with deepest penetration is always kept, and it tries to maximuze the area covered by the points
///note that some pairs of objects might have more then one contact manifold.


ATTRIBUTE_ALIGNED128(class) btPersistentManifold : public btTypedObject
//ATTRIBUTE_ALIGNED16( class) btPersistentManifold : public btTypedObject
{

	btManifoldPoint m_pointCache[MANIFOLD_CACHE_SIZE];

/// this two body pointers can point to the physics rigidbody class.
const btCollisionObject* m_body0;
const btCollisionObject* m_body1;

size_t	m_cachedPoints;

btScalar	m_contactBreakingThreshold;
btScalar	m_contactProcessingThreshold;


// sort cached points so most isolated points come first
int32_t sortCachedPoints(const btManifoldPoint& pt);

int32_t findContactPoint(const btManifoldPoint* unUsed, size_t numUnused, const btManifoldPoint& pt);

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	size_t	m_companionIdA;
	size_t	m_companionIdB;

	size_t m_index1a;

	btPersistentManifold();

	btPersistentManifold(const btCollisionObject* body0, const btCollisionObject* body1, size_t, btScalar contactBreakingThreshold,btScalar contactProcessingThreshold)
		: btTypedObject(BT_PERSISTENT_MANIFOLD_TYPE)
		, m_body0(body0)
		, m_body1(body1)
		, m_cachedPoints(0u),
		m_contactBreakingThreshold(contactBreakingThreshold),
		m_contactProcessingThreshold(contactProcessingThreshold)
	{
	}

	inline const btCollisionObject* getBody0() const { return m_body0; }
	inline const btCollisionObject* getBody1() const { return m_body1; }

	void setBodies(const btCollisionObject* body0,const btCollisionObject* body1) {
		m_body0 = body0;
		m_body1 = body1;
	}

	void clearUserCache(btManifoldPoint& pt);

#ifdef DEBUG_PERSISTENCY
	void DebugPersistency();
#endif //

	inline size_t getNumContacts() const { return m_cachedPoints; }
	/// the setNumContacts API is usually not used, except when you gather/fill all contacts manually
	void setNumContacts(size_t cachedPoints) {
		m_cachedPoints = cachedPoints;
	}

	inline const btManifoldPoint& getContactPoint(size_t index) const {
		btAssert(index < m_cachedPoints);
		return m_pointCache[index];
	}

	inline btManifoldPoint& getContactPoint(size_t index) {
		btAssert(index < m_cachedPoints);
		return m_pointCache[index];
	}

	// @todo: get this margin from the current physics / collision environment
	btScalar getContactBreakingThreshold() const;

	btScalar getContactProcessingThreshold() const {
		return m_contactProcessingThreshold;
	}

	void setContactBreakingThreshold(btScalar contactBreakingThreshold) {
		m_contactBreakingThreshold = contactBreakingThreshold;
	}

	void setContactProcessingThreshold(btScalar	contactProcessingThreshold) {
		m_contactProcessingThreshold = contactProcessingThreshold;
	}

	int32_t getCacheEntry(const btManifoldPoint& newPoint) const;

	size_t addManifoldPoint(const btManifoldPoint& newPoint, bool isPredictive = false);

	void removeContactPoint(size_t index) {
		clearUserCache(m_pointCache[index]);

		size_t lastUsedIndex = getNumContacts() - 1u;
		// m_pointCache[index] = m_pointCache[lastUsedIndex];
		if (index != lastUsedIndex) {
			m_pointCache[index] = m_pointCache[lastUsedIndex];
			//get rid of duplicated userPersistentData pointer
			m_pointCache[lastUsedIndex].m_userPersistentData = nullptr;
			m_pointCache[lastUsedIndex].m_appliedImpulse = 0.f;
			m_pointCache[lastUsedIndex].m_lateralFrictionInitialized = false;
			m_pointCache[lastUsedIndex].m_appliedImpulseLateral1 = 0.f;
			m_pointCache[lastUsedIndex].m_appliedImpulseLateral2 = 0.f;
			m_pointCache[lastUsedIndex].m_lifeTime = 0u;
		}

		btAssert(m_pointCache[lastUsedIndex].m_userPersistentData == nullptr);

		m_cachedPoints--;
	}

	void replaceContactPoint(const btManifoldPoint& newPoint, size_t insertIndex) {
		btAssert(validContactDistance(newPoint));

#define MAINTAIN_PERSISTENCY 1
#ifdef MAINTAIN_PERSISTENCY
		size_t	lifeTime = m_pointCache[insertIndex].getLifeTime();
		btScalar appliedImpulse = m_pointCache[insertIndex].m_appliedImpulse;
		btScalar appliedLateralImpulse1 = m_pointCache[insertIndex].m_appliedImpulseLateral1;
		btScalar appliedLateralImpulse2 = m_pointCache[insertIndex].m_appliedImpulseLateral2;
		// bool isLateralFrictionInitialized = m_pointCache[insertIndex].m_lateralFrictionInitialized;

		btAssert(lifeTime >= 0u);
		void* cache = m_pointCache[insertIndex].m_userPersistentData;

		m_pointCache[insertIndex] = newPoint;

		m_pointCache[insertIndex].m_userPersistentData = cache;
		m_pointCache[insertIndex].m_appliedImpulse = appliedImpulse;
		m_pointCache[insertIndex].m_appliedImpulseLateral1 = appliedLateralImpulse1;
		m_pointCache[insertIndex].m_appliedImpulseLateral2 = appliedLateralImpulse2;

		m_pointCache[insertIndex].m_appliedImpulse = appliedImpulse;
		m_pointCache[insertIndex].m_appliedImpulseLateral1 = appliedLateralImpulse1;
		m_pointCache[insertIndex].m_appliedImpulseLateral2 = appliedLateralImpulse2;

		m_pointCache[insertIndex].m_lifeTime = lifeTime;
#else
		clearUserCache(m_pointCache[insertIndex]);
		m_pointCache[insertIndex] = newPoint;

#endif
		}

	bool validContactDistance(const btManifoldPoint& pt) const {
		return pt.m_distance1 <= getContactBreakingThreshold();
	}

	// calculated new worldspace coordinates and depth, and reject points that exceed the collision margin
	void refreshContactPoints(const btTransform& trA,const btTransform& trB);

	inline void clearManifold() {
		size_t i;

		for (i = 0u; i < m_cachedPoints; i++)
			clearUserCache(m_pointCache[i]);

		m_cachedPoints = 0u;
	}

}
;





#endif //BT_PERSISTENT_MANIFOLD_H
