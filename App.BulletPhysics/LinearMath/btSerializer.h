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

#ifndef BT_SERIALIZER_H
#define BT_SERIALIZER_H

#include "btScalar.h" // has definitions like SIMD_FORCE_INLINE
#include "btHashMap.h"

#if !defined( __CELLOS_LV2__) && !defined(__MWERKS__)
#include <memory.h>
#endif
#include <string.h>

///only the 32bit versions for now
extern int8_t sBulletDNAstr[];
extern int32_t sBulletDNAlen;
extern int8_t sBulletDNAstr64[];
extern int32_t sBulletDNAlen64;

inline size_t btStrLen(const char* str) {
	if (!str)
		return(0u);

	size_t len = 0u;

	while (*str != 0) {
		str++;
		len++;
	}

	return len;
}


class btChunk {
public:
	size_t m_chunkCode;
	size_t m_length;
	void* m_oldPtr;
	size_t m_dna_nr;
	size_t m_number;
};

enum btSerializationFlags {
	BT_SERIALIZE_NO_BVH = 1,
	BT_SERIALIZE_NO_TRIANGLEINFOMAP = 2,
	BT_SERIALIZE_NO_DUPLICATE_ASSERT = 4
};

class btSerializer {
public:
	virtual ~btSerializer() {}

	virtual	const uint8_t* getBufferPointer() const = 0;

	virtual	size_t getCurrentBufferSize() const = 0;

	virtual	btChunk* allocate(size_t size, size_t numElements) = 0;

	virtual	void finalizeChunk(btChunk* chunk, const char* structType, int32_t chunkCode, void* oldPtr) = 0;

	virtual void* findPointer(void* oldPtr) = 0;

	virtual	void* getUniquePointer(void* oldPtr) = 0;

	virtual	void startSerialization() = 0;

	virtual	void finishSerialization() = 0;

	virtual	const char* findNameForPointer(const void* ptr) const = 0;

	virtual	void registerNameForPointer(const void* ptr, const char* name) = 0;

	virtual void serializeName(const char* ptr) = 0;

	virtual uint32_t getSerializationFlags() const = 0;

	virtual void setSerializationFlags(uint32_t flags) = 0;
};



#define BT_HEADER_LENGTH 12u
#if defined(__sgi) || defined (__sparc) || defined (__sparc__) || defined (__PPC__) || defined (__ppc__) || defined (__BIG_ENDIAN__)
#	define BT_MAKE_ID(a,b,c,d) ( (int)(a)<<24 | (int)(b)<<16 | (c)<<8 | (d) )
#else
#	define BT_MAKE_ID(a,b,c,d) ( (int)(d)<<24 | (int)(c)<<16 | (b)<<8 | (a) )
#endif

#define BT_SOFTBODY_CODE		BT_MAKE_ID('S','B','D','Y')
#define BT_COLLISIONOBJECT_CODE BT_MAKE_ID('C','O','B','J')
#define BT_RIGIDBODY_CODE		BT_MAKE_ID('R','B','D','Y')
#define BT_CONSTRAINT_CODE		BT_MAKE_ID('C','O','N','S')
#define BT_BOXSHAPE_CODE		BT_MAKE_ID('B','O','X','S')
#define BT_QUANTIZED_BVH_CODE	BT_MAKE_ID('Q','B','V','H')
#define BT_TRIANLGE_INFO_MAP	BT_MAKE_ID('T','M','A','P')
#define BT_SHAPE_CODE			BT_MAKE_ID('S','H','A','P')
#define BT_ARRAY_CODE			BT_MAKE_ID('A','R','A','Y')
#define BT_SBMATERIAL_CODE		BT_MAKE_ID('S','B','M','T')
#define BT_SBNODE_CODE			BT_MAKE_ID('S','B','N','D')
#define BT_DYNAMICSWORLD_CODE	BT_MAKE_ID('D','W','L','D')
#define BT_DNA_CODE				BT_MAKE_ID('D','N','A','1')


struct	btPointerUid {
	union {
		void* m_ptr;
		uint32_t m_uniqueIds[2];
	};
};

///The btDefaultSerializer is the main Bullet serialization class.
///The constructor takes an optional argument for backwards compatibility, it is recommended to leave this empty/zero.
class btDefaultSerializer : public btSerializer {
	btAlignedObjectArray<char*>		mTypes;
	btAlignedObjectArray<int16_t*>		mStructs;
	btAlignedObjectArray<int16_t>		mTlens;
	btHashMap<btHashInt, int32_t>		mStructReverse;
	btHashMap<btHashString, int32_t>	mTypeLookup;

	btHashMap<btHashPtr, void*>	m_chunkP;

	btHashMap<btHashPtr, const char*>	m_nameMap;

	btHashMap<btHashPtr, btPointerUid>	m_uniquePointers;
	uint32_t m_uniqueIdGenerator;

	size_t m_totalSize;
	uint8_t* m_buffer;
	size_t m_currentSize;
	void* m_dna;
	size_t m_dnaLength;

	size_t m_serializationFlags;

	btAlignedObjectArray<btChunk*>	m_chunkPtrs;

protected:

	virtual	void* findPointer(void* oldPtr) {
		void** ptr = m_chunkP.find(oldPtr);

		if (ptr && *ptr)
			return *ptr;

		return nullptr;
	}

	void writeDNA() {
		btChunk* dnaChunk = allocate(m_dnaLength, 1);

		memcpy(dnaChunk->m_oldPtr, m_dna, m_dnaLength);
		finalizeChunk(dnaChunk, "DNA1", BT_DNA_CODE, m_dna);
	}

	int32_t getReverseType(const char* type) const {
		btHashString key(type);

		const int32_t* valuePtr = mTypeLookup.find(key);

		if (valuePtr)
			return *valuePtr;

		return -1;
	}

	void initDNA(const char* bdnaOrg, size_t dnalen) {
		///was already initialized
		if (m_dna)
			return;

		size_t littleEndian = 1u;
		littleEndian = ((char*)&littleEndian)[0];


		m_dna = btAlignedAlloc(dnalen, 16u);
		memcpy(m_dna, bdnaOrg, dnalen);
		m_dnaLength = dnalen;

		int32_t* intPtr = nullptr;
		int16_t* shtPtr = nullptr;
		char* cp = nullptr;

		size_t dataLen = 0u;

		intPtr = (int32_t*)m_dna;

		/*
			SDNA (4 bytes) (magic number)
			NAME (4 bytes)
			<nr> (4 bytes) amount of names (int)
			<string>
			<string>
		*/

		if (strncmp((const char*)m_dna, "SDNA", 4u) == 0) {
			// skip ++ NAME
			intPtr++; intPtr++;
		}

		// Parse names
		if (!littleEndian)
			*intPtr = btSwapEndian(*intPtr);

		dataLen = *intPtr;

		intPtr++;

		cp = (char*)intPtr;

		size_t i;
		for (i = 0u; i < dataLen; i++) {
			while (*cp)cp++;

			cp++;
		}

		cp = btAlignPointer(cp, 4u);

		/*
			TYPE (4 bytes)
			<nr> amount of types (int)
			<string>
			<string>
		*/

		intPtr = (int32_t*)cp;
		btAssert(strncmp(cp, "TYPE", 4u) == 0); intPtr++;

		if (!littleEndian)
			*intPtr = btSwapEndian(*intPtr);

		dataLen = *intPtr;
		intPtr++;


		cp = (char*)intPtr;

		for (i = 0u; i < dataLen; i++) {
			mTypes.push_back(cp);

			while (*cp)cp++;
			cp++;
		}

		cp = btAlignPointer(cp, 4);

		/*
			TLEN (4 bytes)
			<len> (short) the lengths of types
			<len>
		*/

		// Parse type lens
		intPtr = (int32_t*)cp;
		btAssert(strncmp(cp, "TLEN", 4u) == 0); intPtr++;

		dataLen = mTypes.size();

		shtPtr = (int16_t*)intPtr;
		for (i = 0u; i < dataLen; i++, shtPtr++) {
			if (!littleEndian)
				shtPtr[0] = btSwapEndian(shtPtr[0]);

			mTlens.push_back(shtPtr[0]);
		}

		if (dataLen & 1u) shtPtr++;

		/*
			STRC (4 bytes)
			<nr> amount of structs (int)
			<typenr>
			<nr_of_elems>
			<typenr>
			<namenr>
			<typenr>
			<namenr>
		*/

		intPtr = (int32_t*)shtPtr;
		cp = (char*)intPtr;
		btAssert(strncmp(cp, "STRC", 4u) == 0); intPtr++;

		if (!littleEndian)
			*intPtr = btSwapEndian(*intPtr);

		dataLen = *intPtr;
		intPtr++;

		shtPtr = (int16_t*)intPtr;

		for (i = 0u; i < dataLen; i++) {
			mStructs.push_back(shtPtr);

			if (!littleEndian) {
				shtPtr[0] = btSwapEndian(shtPtr[0]);
				shtPtr[1] = btSwapEndian(shtPtr[1]);

				size_t len = shtPtr[1];
				shtPtr += 2u;

				for (size_t a = 0u; a < len; a++, shtPtr += 2u) {
					shtPtr[0] = btSwapEndian(shtPtr[0]);
					shtPtr[1] = btSwapEndian(shtPtr[1]);
				}

			}
			else
				shtPtr += (2u * shtPtr[1]) + 2u;
		}

		// build reverse lookups
		for (i = 0u; i < mStructs.size(); i++) {
			int16_t* strc = mStructs.at(i);

			mStructReverse.insert(strc[0], i);
			mTypeLookup.insert(btHashString(mTypes[strc[0]]), i);
		}
	}

public:
	btDefaultSerializer(int totalSize = 0)
		:m_totalSize(totalSize),
		m_currentSize(0u),
		m_dna(nullptr),
		m_dnaLength(0u),
		m_serializationFlags(0u)
	{
		m_buffer = m_totalSize ? (uint8_t*)btAlignedAlloc(totalSize, 16u) : 0u;

		const bool VOID_IS_8 = ((sizeof(void*) == 8));

#ifdef BT_INTERNAL_UPDATE_SERIALIZATION_STRUCTURES
		if (VOID_IS_8) {
#if _WIN64
			initDNA((const char*)sBulletDNAstr64, sBulletDNAlen64);
#else
			btAssert(0);
#endif
		}
		else {
#ifndef _WIN64
			initDNA((const char*)sBulletDNAstr, sBulletDNAlen);
#else
			btAssert(0);
#endif
		}

#else //BT_INTERNAL_UPDATE_SERIALIZATION_STRUCTURES
		if (VOID_IS_8)
			initDNA((const char*)sBulletDNAstr64, sBulletDNAlen64);
		else
			initDNA((const char*)sBulletDNAstr, sBulletDNAlen);
#endif //BT_INTERNAL_UPDATE_SERIALIZATION_STRUCTURES

	}

	virtual ~btDefaultSerializer() {
		if (m_buffer)
			btAlignedFree(m_buffer);
		if (m_dna)
			btAlignedFree(m_dna);
	}

	void writeHeader(uint8_t* buffer) const {
#ifdef  BT_USE_DOUBLE_PRECISION
		memcpy(buffer, "BULLETd", 7u);
#else
		memcpy(buffer, "BULLETf", 7u);
#endif

		size_t littleEndian = 1u;
		littleEndian = ((size_t*)&littleEndian)[0];

		if (sizeof(void*) == 8u)
			buffer[7] = '-';
		else
			buffer[7] = '_';

		if (littleEndian)
			buffer[8] = 'v';
		else
			buffer[8] = 'V';

		buffer[9] = '2';
		buffer[10] = '8';
		buffer[11] = '2';
	}

	virtual	void startSerialization() {
		m_uniqueIdGenerator = 1u;

		if (m_totalSize) {
			uint8_t* buffer = internalAlloc(BT_HEADER_LENGTH);
			writeHeader(buffer);
		}

	}

	virtual	void finishSerialization() {
		writeDNA();

		//if we didn't pre-allocate a buffer, we need to create a contiguous buffer now
		size_t mysize = 0u;

		if (!m_totalSize) {
			if (m_buffer)
				btAlignedFree(m_buffer);

			m_currentSize += BT_HEADER_LENGTH;
			m_buffer = (uint8_t*)btAlignedAlloc(m_currentSize, 16u);

			uint8_t* currentPtr = m_buffer;
			writeHeader(m_buffer);
			currentPtr += BT_HEADER_LENGTH;
			mysize += BT_HEADER_LENGTH;

			for (size_t i = 0u; i < m_chunkPtrs.size(); i++) {
				size_t curLength = sizeof(btChunk) + m_chunkPtrs[i]->m_length;

				memcpy(currentPtr, m_chunkPtrs[i], curLength);

				btAlignedFree(m_chunkPtrs[i]);

				currentPtr += curLength;
				mysize += curLength;
			}
		}

		mTypes.clear();
		mStructs.clear();
		mTlens.clear();
		mStructReverse.clear();
		mTypeLookup.clear();
		m_chunkP.clear();
		m_nameMap.clear();
		m_uniquePointers.clear();
		m_chunkPtrs.clear();
	}

	virtual	void* getUniquePointer(void* oldPtr) {
		if (!oldPtr)
			return nullptr;

		btPointerUid* uptr = (btPointerUid*)m_uniquePointers.find(oldPtr);
		if (uptr)
			return uptr->m_ptr;

		m_uniqueIdGenerator++;

		btPointerUid uid;
		uid.m_uniqueIds[0] = m_uniqueIdGenerator;
		uid.m_uniqueIds[1] = m_uniqueIdGenerator;

		m_uniquePointers.insert(oldPtr, uid);

		return uid.m_ptr;
	}

	virtual	const uint8_t* getBufferPointer() const {
		return m_buffer;
	}

	virtual	size_t getCurrentBufferSize() const {
		return	m_currentSize;
	}

	virtual void finalizeChunk(btChunk* chunk, const char* structType, int32_t chunkCode, void* oldPtr) {
		if (!(m_serializationFlags & BT_SERIALIZE_NO_DUPLICATE_ASSERT))
			btAssert(!findPointer(oldPtr));

		chunk->m_dna_nr = getReverseType(structType);

		chunk->m_chunkCode = chunkCode;

		void* uniquePtr = getUniquePointer(oldPtr);

		m_chunkP.insert(oldPtr, uniquePtr);//chunk->m_oldPtr);
		chunk->m_oldPtr = uniquePtr;//oldPtr;
	}

	virtual uint8_t* internalAlloc(size_t size) {
		uint8_t* ptr = 0u;

		if (m_totalSize) {
			ptr = m_buffer + m_currentSize;
			m_currentSize += size;

			btAssert(m_currentSize < m_totalSize);
		}
		else {
			ptr = (unsigned char*)btAlignedAlloc(size, 16u);

			m_currentSize += size;
		}

		return ptr;
	}

	virtual	btChunk* allocate(size_t size, size_t numElements) {
		uint8_t* ptr = internalAlloc(size * numElements + sizeof(btChunk));

		uint8_t* data = ptr + sizeof(btChunk);

		btChunk* chunk = (btChunk*)ptr;
		chunk->m_chunkCode = 0u;
		chunk->m_oldPtr = data;
		chunk->m_length = size * numElements;
		chunk->m_number = numElements;

		m_chunkPtrs.push_back(chunk);

		return chunk;
	}

	virtual	const char* findNameForPointer(const void* ptr) const {
		const char* const* namePtr = m_nameMap.find(ptr);

		if (namePtr && *namePtr)
			return *namePtr;

		return 0;
	}

	virtual	void registerNameForPointer(const void* ptr, const char* name) {
		m_nameMap.insert(ptr, name);
	}

	virtual void serializeName(const char* name) {
		if (name) {
			//don't serialize name twice
			if (findPointer((void*)name))
				return;

			size_t len = btStrLen(name);

			if (len) {
				size_t newLen = len + 1u;
				size_t padding = ((newLen + 3u) & ~3u) - newLen;
				newLen += padding;

				//serialize name string now
				btChunk* chunk = allocate(sizeof(char), newLen);
				char* destinationName = (char*)chunk->m_oldPtr;

				for (size_t i = 0u; i < len; i++)
					destinationName[i] = name[i];

				destinationName[len] = 0;
				finalizeChunk(chunk, "char", BT_ARRAY_CODE, (void*)name);
			}
		}
	}

	virtual uint32_t getSerializationFlags() const {
		return m_serializationFlags;
	}

	virtual void setSerializationFlags(uint32_t flags) {
		m_serializationFlags = flags;
	}

};

#endif //BT_SERIALIZER_H

