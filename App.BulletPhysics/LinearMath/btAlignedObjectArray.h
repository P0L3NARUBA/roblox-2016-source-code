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


#ifndef BT_OBJECT_ARRAY__
#define BT_OBJECT_ARRAY__

#include "btScalar.h" // has definitions like SIMD_FORCE_INLINE
#include "btAlignedAllocator.h"

///If the platform doesn't support placement new, you can disable BT_USE_PLACEMENT_NEW
///then the btAlignedObjectArray doesn't support objects with virtual methods, and non-trivial constructors/destructors
///You can enable BT_USE_MEMCPY, then swapping elements in the array will use memcpy instead of operator=
///see discussion here: http://continuousphysics.com/Bullet/phpBB2/viewtopic.php?t=1231 and
///http://www.continuousphysics.com/Bullet/phpBB2/viewtopic.php?t=1240

#define BT_USE_PLACEMENT_NEW 1
//#define BT_USE_MEMCPY 1 //disable, because it is cumbersome to find out for each platform where memcpy is defined. It can be in <memory.h> or <string.h> or otherwise...
#define BT_ALLOW_ARRAY_COPY_OPERATOR // enabling this can accidently perform deep copies of data if you are not careful

#ifdef BT_USE_MEMCPY
#include <memory.h>
#include <string.h>
#endif //BT_USE_MEMCPY

#ifdef BT_USE_PLACEMENT_NEW
#include <new> //for placement new
#endif //BT_USE_PLACEMENT_NEW


///The btAlignedObjectArray template class uses a subset of the stl::vector interface for its methods
///It is developed to replace stl::vector to avoid portability issues, including STL alignment issues to add SIMD/SSE data
template <typename T>
//template <class T> 
class btAlignedObjectArray {
	btAlignedAllocator<T, 16u> m_allocator;

	size_t m_size;
	size_t m_capacity;
	T* m_data;
	//PCK: added this line
	bool m_ownsMemory;

#ifdef BT_ALLOW_ARRAY_COPY_OPERATOR
public:
	inline  btAlignedObjectArray<T>& operator=(const btAlignedObjectArray<T>& other) {
		copyFromArray(other);

		return *this;
	}
#else
private:
	SIMD_FORCE_INLINE btAlignedObjectArray<T>& operator=(const btAlignedObjectArray<T>& other);
#endif

protected:
	inline size_t allocSize(size_t size) {
		return (size ? size * 2u : 1u);
	}

	inline void	copy(size_t start, size_t end, T* dest) const {
		size_t i;

		for (i = start; i < end; ++i)
#ifdef BT_USE_PLACEMENT_NEW
			new (&dest[i]) T(m_data[i]);
#else
			dest[i] = m_data[i];
#endif
	}

	inline void init() {
		//PCK: added this line
		m_ownsMemory = true;
		m_data = nullptr;
		m_size = 0u;
		m_capacity = 0u;
	}

	inline void destroy(size_t first, size_t last) {
		size_t i;

		for (i = first; i < last; i++)
			m_data[i].~T();
	}

	inline void* allocate(size_t size) {
		if (size)
			return m_allocator.allocate(size);

		return nullptr;
	}

	inline void deallocate() {
		if (m_data) {
			//PCK: enclosed the deallocation in this block
			if (m_ownsMemory)
				m_allocator.deallocate(m_data);

			m_data = nullptr;
		}
	}

public:

	btAlignedObjectArray() {
		init();
	}

	~btAlignedObjectArray() {
		clear();
	}

	///Generally it is best to avoid using the copy constructor of an btAlignedObjectArray, and use a (const) reference to the array instead.
	btAlignedObjectArray(const btAlignedObjectArray& otherArray) {
		init();

		size_t otherSize = otherArray.size();
		resize(otherSize);
		otherArray.copy(0u, otherSize, m_data);
	}

	/// return the number of elements in the array
	inline size_t size() const {
		return m_size;
	}

	inline const T& at(size_t n) const {
		btAssert(n >= 0u);
		btAssert(n < size());

		return m_data[n];
	}

	inline T& at(size_t n) {
		btAssert(n >= 0u);
		btAssert(n < size());

		return m_data[n];
	}

	inline const T& operator[](size_t n) const {
		btAssert(n >= 0u);
		btAssert(n < size());

		return m_data[n];
	}

	inline T& operator[](size_t n) {
		btAssert(n >= 0u);
		btAssert(n < size());

		return m_data[n];
	}


	///clear the array, deallocated memory. Generally it is better to use array.resize(0), to reduce performance overhead of run-time memory (de)allocations.
	inline void	clear() {
		destroy(0u, size());

		deallocate();

		init();
	}

	inline void pop_back() {
		btAssert(m_size > 0u);

		m_size--;
		m_data[m_size].~T();
	}


	///resize changes the number of elements in the array. If the new size is larger, the new elements will be constructed using the optional second argument.
	///when the new number of elements is smaller, the destructor will be called, but memory will not be freed, to reduce performance overhead of run-time memory (de)allocations.
	inline void resizeNoInitialize(size_t newsize) {
		size_t curSize = size();

		if (newsize < curSize) {

		}
		else {
			if (newsize > size())
				reserve(newsize);

			//leave this uninitialized
		}

		m_size = newsize;
	}

	inline void resize(size_t newsize, const T& fillData = T()) {
		size_t curSize = size();

		if (newsize < curSize) {
			for (size_t i = newsize; i < curSize; i++)
				m_data[i].~T();
		}
		else {
			if (newsize > size())
				reserve(newsize);

#ifdef BT_USE_PLACEMENT_NEW
			for (size_t i = curSize; i < newsize; i++)
				new (&m_data[i]) T(fillData);
#endif
		}

		m_size = newsize;
	}

	inline 	T& expandNonInitializing() {
		size_t sz = size();

		if (sz == capacity())
			reserve(allocSize(size()));

		m_size++;

		return m_data[sz];
	}

	inline T& expand(const T& fillValue = T()) {
		size_t sz = size();

		if (sz == capacity())
			reserve(allocSize(size()));

		m_size++;
#ifdef BT_USE_PLACEMENT_NEW
		new (&m_data[sz]) T(fillValue); //use the in-place new (not really allocating heap memory)
#endif
		return m_data[sz];
	}

	inline void push_back(const T& _Val) {
		int sz = size();
		if (sz == capacity())
			reserve(allocSize(size()));

#ifdef BT_USE_PLACEMENT_NEW
		new (&m_data[m_size]) T(_Val);
#else
		m_data[size()] = _Val;
#endif

		m_size++;
	}


	/// return the pre-allocated (reserved) elements, this is at least as large as the total number of elements,see size() and reserve()
	inline size_t capacity() const {
		return m_capacity;
	}

	inline void reserve(size_t _Count) {	// determine new minimum length of allocated storage
		if (capacity() < _Count) {	// not enough room, reallocate
			T* s = (T*)allocate(_Count);

			copy(0u, size(), s);

			destroy(0u, size());

			deallocate();

			//PCK: added this line
			m_ownsMemory = true;

			m_data = s;

			m_capacity = _Count;
		}
	}


	class less {
	public:

		bool operator() (const T& a, const T& b) {
			return (a < b);
		}
	};


	template <typename L>
	void quickSortInternal(const L& CompareFunc, size_t lo, size_t hi) {
		//  lo is the lower index, hi is the upper index
		//  of the region of array a that is to be sorted
		size_t i = lo;
		size_t j = hi;
		T x = m_data[(lo + hi) / 2u];

		//  partition
		do {
			while (CompareFunc(m_data[i], x))
				i++;
			while (CompareFunc(x, m_data[j]))
				j--;

			if (i <= j) {
				swap(i, j);

				i++; j--;
			}
		} while (i <= j);

		//  recursion
		if (lo < j)
			quickSortInternal(CompareFunc, lo, j);
		if (i < hi)
			quickSortInternal(CompareFunc, i, hi);
	}


	template <typename L>
	void quickSort(const L& CompareFunc) {
		//don't sort 0 or 1 elements
		if (size() > 1u)
			quickSortInternal(CompareFunc, 0u, size() - 1u);
	}


	///heap sort from http://www.csse.monash.edu.au/~lloyd/tildeAlgDS/Sort/Heap/
	template <typename L>
	void downHeap(T* pArr, size_t k, size_t n, const L& CompareFunc) {
		/*  PRE: a[k+1..N] is a heap */
		/* POST:  a[k..N]  is a heap */

		T temp = pArr[k - 1u];
		/* k has child(s) */
		while (k <= n / 2u) {
			size_t child = 2u * k;

			if ((child < n) && CompareFunc(pArr[child - 1u], pArr[child]))
				child++;

			/* pick larger child */
			if (CompareFunc(temp, pArr[child - 1u])) {
				/* move child up */
				pArr[k - 1] = pArr[child - 1u];
				k = child;
			}
			else
				break;
		}
		pArr[k - 1u] = temp;
	} /*downHeap*/

	void swap(size_t index0, size_t index1) {
#ifdef BT_USE_MEMCPY
		char temp[sizeof(T)];
		memcpy(temp, &m_data[index0], sizeof(T));
		memcpy(&m_data[index0], &m_data[index1], sizeof(T));
		memcpy(&m_data[index1], temp, sizeof(T));
#else
		T temp = m_data[index0];

		m_data[index0] = m_data[index1];
		m_data[index1] = temp;
#endif
	}

	template <typename L>
	void heapSort(const L& CompareFunc) {
		/* sort a[0..N-1],  N.B. 0 to N-1 */
		size_t k;
		size_t n = m_size;
		for (k = n / 2u; k > 0u; k--) {
			downHeap(m_data, k, n, CompareFunc);
		}

		/* a[1..N] is now a heap */
		while (n >= 1u) {
			swap(0u, n - 1u); /* largest of a[0..n-1] */

			n = n - 1u;
			/* restore a[1..i-1] heap */
			downHeap(m_data, 1u, n, CompareFunc);
		}
	}

	///non-recursive binary search, assumes sorted array
	size_t findBinarySearch(const T& key) const {
		size_t first = 0u;
		size_t last = size() - 1u;

		//assume sorted array
		while (first <= last) {
			size_t mid = (first + last) / 2u;  // compute mid point.
			if (key > m_data[mid])
				first = mid + 1u;  // repeat search in top half.
			else if (key < m_data[mid])
				last = mid - 1u; // repeat search in bottom half.
			else
				return mid;     // found it. return position /////
		}

		return size();    // failed to find key
	}


	size_t	findLinearSearch(const T& key) const {
		size_t index = size();
		size_t i;

		for (i = 0u; i < size(); i++) {
			if (m_data[i] == key) {
				index = i;

				break;
			}
		}

		return index;
	}

	void remove(const T& key) {
		size_t findIndex = findLinearSearch(key);

		if (findIndex < size()) {
			swap(findIndex, size() - 1u);
			pop_back();
		}
	}

	//PCK: whole function
	void initializeFromBuffer(void* buffer, size_t size, size_t capacity) {
		clear();

		m_ownsMemory = false;
		m_data = (T*)buffer;
		m_size = size;
		m_capacity = capacity;
	}

	void copyFromArray(const btAlignedObjectArray& otherArray) {
		size_t otherSize = otherArray.size();

		resize(otherSize);

		otherArray.copy(0u, otherSize, m_data);
	}

};

#endif //BT_OBJECT_ARRAY__
