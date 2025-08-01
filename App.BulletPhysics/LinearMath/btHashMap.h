/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2009 Erwin Coumans  http://bulletphysics.org

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/


#ifndef BT_HASH_MAP_H
#define BT_HASH_MAP_H

#include "btAlignedObjectArray.h"

///very basic hashable string implementation, compatible with btHashMap
struct btHashString {
	const char* m_string;
	size_t		m_hash;

	inline size_t getHash()const {
		return m_hash;
	}

	btHashString(const char* name)
		:m_string(name)
	{
		/* magic numbers from http://www.isthe.com/chongo/tech/comp/fnv/ */
		static const size_t InitialFNV = 2166136261u;
		static const size_t FNVMultiple = 16777619u;

		/* Fowler / Noll / Vo (FNV) Hash */
		size_t hash = InitialFNV;

		for (size_t i = 0u; m_string[i]; i++) {
			hash = hash ^ (m_string[i]);       /* xor  the low 8 bits */
			hash = hash * FNVMultiple;  /* multiply by the magic number */
		}

		m_hash = hash;
	}

	int32_t portableStringCompare(const char* src, const char* dst) const {
		int32_t ret = 0;

		while (!(ret = *(unsigned char*)src - *(unsigned char*)dst) && *dst)
			++src, ++dst;

		if (ret < 0)
			ret = -1;
		else if (ret > 0)
			ret = 1;

		return(ret);
	}

	bool equals(const btHashString& other) const {
		return (m_string == other.m_string) || (0u == portableStringCompare(m_string, other.m_string));

	}

};

const size_t BT_HASH_NULL = 0xffffffff;

class btHashInt {
	size_t m_uid;
public:
	btHashInt(size_t uid) :m_uid(uid)
	{
	}

	size_t	getUid1() const {
		return m_uid;
	}

	void setUid1(size_t uid) {
		m_uid = uid;
	}

	bool equals(const btHashInt& other) const {
		return getUid1() == other.getUid1();
	}
	//to our success
	inline size_t getHash() const {
		size_t key = m_uid;
		// Thomas Wang's hash
		key += ~(key << 15u);
		key ^= (key >> 10u);
		key += (key << 3u);
		key ^= (key >> 6u);
		key += ~(key << 11u);
		key ^= (key >> 16u);

		return key;
	}
};



class btHashPtr {
	union {
		const void* m_pointer;
		uint32_t	m_hashValues[2];
	};

public:
	btHashPtr(const void* ptr)
		:m_pointer(ptr)
	{
	}

	const void* getPointer() const {
		return m_pointer;
	}

	bool equals(const btHashPtr& other) const {
		return getPointer() == other.getPointer();
	}

	//to our success
	inline size_t getHash() const {
		const bool VOID_IS_8 = ((sizeof(void*) == 8));

		size_t key = size_t(VOID_IS_8 ? m_hashValues[0] + m_hashValues[1] : m_hashValues[0]);

		// Thomas Wang's hash
		key += ~(key << 15u);
		key ^= (key >> 10u);
		key += (key << 3u);
		key ^= (key >> 6u);
		key += ~(key << 11u);
		key ^= (key >> 16u);

		return key;
	}


};


template <class Value>
class btHashKeyPtr {
	size_t m_uid;
public:

	btHashKeyPtr(int uid) :m_uid(uid)
	{
	}

	size_t getUid1() const {
		return m_uid;
	}

	bool equals(const btHashKeyPtr<Value>& other) const {
		return getUid1() == other.getUid1();
	}

	//to our success
	inline size_t getHash() const {
		size_t key = m_uid;
		// Thomas Wang's hash
		key += ~(key << 15u);
		key ^= (key >> 10u);
		key += (key << 3u);
		key ^= (key >> 6u);
		key += ~(key << 11u);
		key ^= (key >> 16u);

		return key;
	}


};


template <class Value>
class btHashKey {
	size_t	m_uid;
public:

	btHashKey(size_t uid) :m_uid(uid)
	{
	}

	size_t	getUid1() const {
		return m_uid;
	}

	bool equals(const btHashKey<Value>& other) const {
		return getUid1() == other.getUid1();
	}
	//to our success
	inline size_t getHash()const {
		size_t key = m_uid;
		// Thomas Wang's hash
		key += ~(key << 15u);
		key ^= (key >> 10u);
		key += (key << 3u);
		key ^= (key >> 6u);
		key += ~(key << 11u);
		key ^= (key >> 16u);

		return key;
	}
};


///The btHashMap template class implements a generic and lightweight hashmap.
///A basic sample of how to use btHashMap is located in Demos\BasicDemo\main.cpp
template <class Key, class Value>
class btHashMap {
protected:
	btAlignedObjectArray<size_t>	m_hashTable;
	btAlignedObjectArray<size_t>	m_next;

	btAlignedObjectArray<Value>		m_valueArray;
	btAlignedObjectArray<Key>		m_keyArray;

	void growTables(const Key& /*key*/) {
		size_t newCapacity = m_valueArray.capacity();

		if (m_hashTable.size() < newCapacity) {
			//grow hashtable and next table
			size_t curHashtableSize = m_hashTable.size();

			m_hashTable.resize(newCapacity);
			m_next.resize(newCapacity);

			size_t i;

			for (i = 0u; i < newCapacity; ++i)
				m_hashTable[i] = BT_HASH_NULL;

			for (i = 0u; i < newCapacity; ++i)
				m_next[i] = BT_HASH_NULL;

			for (i = 0u; i < curHashtableSize; i++) {
				//const Value& value = m_valueArray[i];
				//const Key& key = m_keyArray[i];

				size_t hashValue = m_keyArray[i].getHash() & (m_valueArray.capacity() - 1u); // New hash value with new mask

				m_next[i] = m_hashTable[hashValue];
				m_hashTable[hashValue] = i;
			}


		}
	}

public:

	void insert(const Key& key, const Value& value) {
		size_t hash = key.getHash() & (m_valueArray.capacity() - 1u);

		//replace value if the key is already there
		size_t index = findIndex(key);
		if (index != BT_HASH_NULL) {
			m_valueArray[index] = value;

			return;
		}

		size_t count = m_valueArray.size();
		size_t oldCapacity = m_valueArray.capacity();
		m_valueArray.push_back(value);
		m_keyArray.push_back(key);

		size_t newCapacity = m_valueArray.capacity();
		if (oldCapacity < newCapacity) {
			growTables(key);
			//hash with new capacity
			hash = key.getHash() & (m_valueArray.capacity() - 1u);
		}

		m_next[count] = m_hashTable[hash];
		m_hashTable[hash] = count;
	}

	void remove(const Key& key) {
		size_t hash = key.getHash() & (m_valueArray.capacity() - 1u);

		size_t pairIndex = findIndex(key);

		if (pairIndex == BT_HASH_NULL)
			return;

		// Remove the pair from the hash table.
		size_t index = m_hashTable[hash];
		btAssert(index != BT_HASH_NULL);

		size_t previous = BT_HASH_NULL;
		while (index != pairIndex) {
			previous = index;
			index = m_next[index];
		}

		if (previous != BT_HASH_NULL) {
			btAssert(m_next[previous] == pairIndex);
			m_next[previous] = m_next[pairIndex];
		}
		else {
			m_hashTable[hash] = m_next[pairIndex];
		}

		// We now move the last pair into spot of the
		// pair being removed. We need to fix the hash
		// table indices to support the move.

		size_t lastPairIndex = m_valueArray.size() - 1u;

		// If the removed pair is the last pair, we are done.
		if (lastPairIndex == pairIndex) {
			m_valueArray.pop_back();
			m_keyArray.pop_back();

			return;
		}

		// Remove the last pair from the hash table.
		size_t lastHash = m_keyArray[lastPairIndex].getHash() & (m_valueArray.capacity() - 1u);

		index = m_hashTable[lastHash];
		btAssert(index != BT_HASH_NULL);

		previous = BT_HASH_NULL;
		while (index != lastPairIndex) {
			previous = index;
			index = m_next[index];
		}

		if (previous != BT_HASH_NULL) {
			btAssert(m_next[previous] == lastPairIndex);
			m_next[previous] = m_next[lastPairIndex];
		}
		else {
			m_hashTable[lastHash] = m_next[lastPairIndex];
		}

		// Copy the last pair into the remove pair's spot.
		m_valueArray[pairIndex] = m_valueArray[lastPairIndex];
		m_keyArray[pairIndex] = m_keyArray[lastPairIndex];

		// Insert the last pair into the hash table
		m_next[pairIndex] = m_hashTable[lastHash];
		m_hashTable[lastHash] = pairIndex;

		m_valueArray.pop_back();
		m_keyArray.pop_back();
	}


	size_t size() const {
		return m_valueArray.size();
	}

	const Value* getAtIndex(size_t index) const {
		btAssert(index < m_valueArray.size());

		return &m_valueArray[index];
	}

	Value* getAtIndex(size_t index) {
		btAssert(index < m_valueArray.size());

		return &m_valueArray[index];
	}

	Value* operator[](const Key& key) {
		return find(key);
	}

	const Value* find(const Key& key) const {
		size_t index = findIndex(key);

		if (index == BT_HASH_NULL)
			return nullptr;

		return &m_valueArray[index];
	}

	Value* find(const Key& key) {
		size_t index = findIndex(key);

		if (index == BT_HASH_NULL)
			return nullptr;

		return &m_valueArray[index];
	}

	size_t	findIndex(const Key& key) const {
		size_t hash = key.getHash() & (m_valueArray.capacity() - 1u);

		if (hash >= m_hashTable.size())
			return BT_HASH_NULL;

		size_t index = m_hashTable[hash];
		while ((index != BT_HASH_NULL) && key.equals(m_keyArray[index]) == false)
			index = m_next[index];
		
		return index;
	}

	void clear() {
		m_hashTable.clear();
		m_next.clear();
		m_valueArray.clear();
		m_keyArray.clear();
	}

};

#endif //BT_HASH_MAP_H
