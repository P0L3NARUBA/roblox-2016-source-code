/**
  @file Array.h

  @maintainer Morgan McGuire, graphics3d.com
  @cite Portions written by Aaron Orenstein, a@orenstein.name

  @created 2001-03-11
  @edited  2009-05-29

  Copyright 2000-2009, Morgan McGuire, http://graphics.cs.williams.edu
  All rights reserved.
 */

#ifndef G3D_Array_h
#define G3D_Array_h

#include "G3D/platform.h"
#include "G3D/debug.h"
#include "G3D/System.h"
#include "G3D/MemoryManager.h"
#ifdef G3D_DEBUG
 //   For formatting error messages
#    include "G3D/format.h"
#endif
#include <vector>
#include <algorithm>

#ifdef _MSC_VER
#   include <new>

#   pragma warning (push)
	// debug information too long
#   pragma warning( disable : 4312)
#   pragma warning( disable : 4786)
#endif

#if !defined(G3D_IOS) && !defined(__ANDROID__)
#define CALLSPEC __cdecl
#else
#define CALLSPEC
#endif
namespace G3D {

	/**
	 Constant for passing to Array::resize
	 */
	const bool DONT_SHRINK_UNDERLYING_ARRAY = false;

	/** Constant for Array::sort */
	const size_t SORT_INCREASING = 1;
	/** Constant for Array::sort */
	//const int8_t SORT_DECREASING = -1;

	/**
	 \brief Dynamic 1D array tuned for performance.

	 Objects must have a default constructor (constructor that
	 takes no arguments) in order to be used with this template.
	 You will get the error "no appropriate default constructor found"
	 if they do not.

	 Do not use with objects that overload placement <code>operator new</code>,
	 since the speed of Array is partly due to pooled allocation.

	 Array is highly optimized compared to std::vector.
	 Array operations are less expensive than on std::vector and for large
	 amounts of data, Array consumes only 1.5x the total size of the
	 data, while std::vector consumes 2.0x.  The default
	 array takes up zero heap space.  The first resize (or append)
	 operation grows it to a reasonable internal size so it is efficient
	 to append to small arrays.

	 Then Array needs to copy
	 data internally on a resize operation it correctly invokes copy
	 constructors of the elements (the MSVC6 implementation of
	 std::vector uses realloc, which can create memory leaks for classes
	 containing references and pointers).  Array provides a guaranteed
	 safe way to access the underlying data as a flat C array --
	 Array::getCArray.  Although (T*)std::vector::begin() can be used for
	 this purpose, it is not guaranteed to succeed on all platforms.

	 To serialize an array, see G3D::serialize.

	 The template parameter MIN_ELEMENTS indicates the smallest number of
	 elements that will be allocated.  The default of 10 is designed to avoid
	 the overhead of repeatedly allocating the array as it grows from 1, to 2, and so on.
	 If you are creating a lot of small Arrays, however, you may want to set this smaller
	 to reduce the memory cost. Once the array has been allocated, it will never
	 deallocate the underlying array unless MIN_ELEMENTS is set to 0, MIN_BYTES is 0, and the array
	 is empty.

	 Do not subclass an Array.

	 \sa G3D::SmallArray
	 */
	template <class T, size_t MIN_ELEMENTS = 10u, size_t MIN_BYTES = 32u>
	class Array {
	private:
		/** 0...num-1 are initialized elements, num...numAllocated-1 are not */
		T* data;

		size_t num;
		size_t numAllocated;

		MemoryManager::Ref  m_memoryManager;

		/** \param n Number of elements
		*/
		void init(size_t n, const MemoryManager::Ref& m) {
			m_memoryManager = m;

			debugAssert(n >= 0u);

			this->num = 0u;
			this->numAllocated = 0u;

			data = nullptr;

			if (n > 0u) {
				resize(n);
			}
			else {
				data = nullptr;
			}
		}

		void _copy(const Array& other) {
			init(other.num, MemoryManager::create());

			for (size_t i = 0u; i < num; i++) {
				data[i] = other.data[i];
			}
		}

		/**
		 Returns true iff address points to an element of this array.
		 Used by append.
		 */
		inline bool inArray(const T* address) {
			return (address >= data) && (address < data + num);
		}


		/** Only compiled if you use the sort procedure. */
		static bool CALLSPEC compareGT(const T& a, const T& b) {
			return a > b;
		}


		/**
		 Allocates a new array of size numAllocated (not a parameter to the method)
		 and then copies at most oldNum elements from the old array to it.  Destructors are
		 called for oldNum elements of the old array.
		 */
		void realloc(size_t oldNum) {
			T* oldData = data;

			// The allocation is separate from the constructor invocation because we don't want 
			// to pay for the cost of constructors until the newly allocated
			// elements are actually revealed to the application.  They 
			// will be constructed in the resize() method.

			data = (T*)m_memoryManager->alloc(sizeof(T) * numAllocated);
			alwaysAssertM(data, "Memory manager returned NULL: out of memory?");

			// Call the copy constructors
			{
				const size_t N = G3D::min(oldNum, numAllocated);
				const T* end = data + N;
				T* oldPtr = oldData;

				for (T* ptr = data; ptr < end; ++ptr, ++oldPtr) {
					// Use placement new to invoke the constructor at the location
					// that we determined.  Use the copy constructor to make the assignment.
					const T* constructed = new (ptr) T(*oldPtr);

					(void)constructed;
					debugAssertM(constructed == ptr,
						"new returned a different address than the one provided by Array.");
				}
			}

			// Call destructors on the old array (if there is no destructor, this will compile away)
			{
				const T* end = oldData + oldNum;

				for (T* ptr = oldData; ptr < end; ++ptr) {
					ptr->~T();
				}
			}

			m_memoryManager->free(oldData);
		}

	public:

		/**
		 G3D C++ STL style iterator variable.  Call begin() to get
		 the first iterator, pre-increment (++i) the iterator to get to
		 the next value.  Use dereference (*i) to access the element.
		 */
		typedef T* Iterator;
		/** G3D C++ STL style const iterator in same style as Iterator. */
		typedef const T* ConstIterator;

		/** stl porting compatibility helper */
		typedef Iterator iterator;
		/** stl porting compatibility helper */
		typedef ConstIterator const_iterator;
		/** stl porting compatibility helper */
		typedef T value_type;
		/** stl porting compatibility helper */
		typedef size_t size_type;
		/** stl porting compatibility helper */
		typedef size_t difference_type;

		/**
		 C++ STL style iterator method.  Returns the first iterator element.
		 Do not change the size of the array while iterating.
		 */
		Iterator begin() {
			return data;
		}

		ConstIterator begin() const {
			return data;
		}
		/**
		 C++ STL style iterator method.  Returns one after the last iterator
		 element.
		 */
		ConstIterator end() const {
			return data + num;
		}

		Iterator end() {
			return data + num;
		}

		/**
		 The array returned is only valid until the next append() or resize call, or
		 the Array is deallocated.
		 */
		T* getCArray() {
			return data;
		}

		/**
		 The array returned is only valid until the next append() or resize call, or
		 the Array is deallocated.
		 */
		const T* getCArray() const {
			return data;
		}

		/** Creates a zero length array (no heap allocation occurs until resize). */
		Array() : num(0u) {
			init(0u, MemoryManager::create());
			debugAssert(num >= 0u);
		}


		/**  Creates an array containing v0. */
		Array(const T& v0) {
			init(1u, MemoryManager::create());
			(*this)[0] = v0;
		}

		/**  Creates an array containing v0 and v1. */
		Array(const T& v0, const T& v1) {
			init(2u, MemoryManager::create());
			(*this)[0] = v0;
			(*this)[1] = v1;
		}

		/**  Creates an array containing v0...v2. */
		Array(const T& v0, const T& v1, const T& v2) {
			init(3u, MemoryManager::create());
			(*this)[0] = v0;
			(*this)[1] = v1;
			(*this)[2] = v2;
		}

		/** Creates an array containing v0...v3. */
		Array(const T& v0, const T& v1, const T& v2, const T& v3) {
			init(4u, MemoryManager::create());
			(*this)[0] = v0;
			(*this)[1] = v1;
			(*this)[2] = v2;
			(*this)[3] = v3;
		}

		/** Creates an array containing v0...v4. */
		Array(const T& v0, const T& v1, const T& v2, const T& v3, const T& v4) {
			init(5u, MemoryManager::create());
			(*this)[0] = v0;
			(*this)[1] = v1;
			(*this)[2] = v2;
			(*this)[3] = v3;
			(*this)[4] = v4;
		}


		/**
		 Copy constructor
		 */
		Array(const Array& other) : num(0u) {
			_copy(other);
			debugAssert(num >= 0u);
		}

		/**
		 Destructor does not delete() the objects if T is a pointer type
		 (e.g. T = int*) instead, it deletes the <B>pointers themselves</B> and
		 leaves the objects.  Call deleteAll if you want to dealocate
		 the objects referenced.  Do not call deleteAll if <CODE>T</CODE> is not a pointer
		 type (e.g. do call Array<Foo*>::deleteAll, do <B>not</B> call Array<Foo>::deleteAll).
		 */
		~Array() {
			// Invoke the destructors on the elements
			for (size_t i = 0u; i < num; i++) {
				(data + i)->~T();
			}

			m_memoryManager->free(data);
			// Set to 0 in case this Array is global and gets referenced during app exit
			data = nullptr;
			num = 0u;
			numAllocated = 0u;
		}

		/**
		 Removes all elements.  Use resize(0, false) or fastClear if you want to
		 remove all elements without deallocating the underlying array
		 so that future append() calls will be faster.
		 */
		void clear(bool shrink = true) {
			resize(0u, shrink);
		}

		void clearAndSetMemoryManager(const MemoryManager::Ref& m) {
			clear();

			debugAssert(data == nullptr);

			m_memoryManager = m;
		}

		/** resize(0, false)
		   @deprecated*/
		void fastClear() {
			clear(false);
		}

		/**
		 Assignment operator.
		 */
		Array& operator=(const Array& other) {
			debugAssert(num >= 0u);

			resize(other.num);

			for (size_t i = 0u; i < num; ++i) {
				data[i] = other[i];
			}

			debugAssert(num >= 0u);

			return *this;
		}

		Array& operator=(const std::vector<T>& other) {
			resize(other.size());

			for (size_t i = 0u; i < num; ++i) {
				data[i] = other[i];
			}

			return *this;
		}

		inline MemoryManager::Ref memoryManager() const {
			return m_memoryManager;
		}

		/**
		 Number of elements in the array.
		 */
		inline size_t size() const {
			return num;
		}

		/**
		 Number of elements in the array.  (Same as size; this is just
		 here for convenience).
		 */
		inline size_t length() const {
			return size();
		}

		/**
		 Swaps element index with the last element in the array then
		 shrinks the array by one.
		 */
		void fastRemove(size_t index, bool shrinkIfNecessary = false) {
			debugAssert(index >= 0u);
			debugAssert(index < num);

			data[index] = data[num - 1u];

			resize(size() - 1u, shrinkIfNecessary);
		}


		/**
		 Inserts at the specified index and shifts all other elements up by one.
		 */
		void insert(size_t n, const T& value) {
			// Add space for the extra element
			resize(num + 1u, false);

			for (size_t i = num - 1u; i > n; --i) {
				data[i] = data[i - 1u];
			}

			data[n] = value;
		}

		/** @param shrinkIfNecessary if false, memory will never be
		  reallocated when the array shrinks.  This makes resizing much
		  faster but can waste memory.
		*/
		void resize(int n, bool shrinkIfNecessary = true) {
			debugAssert(n >= 0u);

			if (num == n) {
				return;
			}

			size_t oldNum = num;
			num = n;

			// Call the destructors on newly hidden elements if there are any
			for (size_t i = num; i < oldNum; ++i) {
				(data + i)->~T();
			}

			// Once allocated, always maintain MIN_ELEMENTS elements or 32 bytes, whichever is higher.
			const size_t minSize = std::max(MIN_ELEMENTS, (MIN_BYTES / sizeof(T)));

			if ((MIN_ELEMENTS == 0u) && (MIN_BYTES == 0u) && (n == 0u) && shrinkIfNecessary) {
				// Deallocate the array completely
				numAllocated = 0u;
				m_memoryManager->free(data);
				data = nullptr;

				return;
			}

			if (num > numAllocated) {
				// Grow the underlying array

				if (numAllocated == 0u) {
					// First allocation; grow to exactly the size requested to avoid wasting space.
					numAllocated = n;

					debugAssert(oldNum == 0u);

					realloc(oldNum);
				}
				else {
					if (num < minSize) {
						// Grow to at least the minimum size
						numAllocated = minSize;

					}
					else {

						// Increase the underlying size of the array.  Grow aggressively
						// up to 64k, less aggressively up to 400k, and then grow relatively
						// slowly (1.5x per resize) to avoid excessive space consumption.
						//
						// These numbers are tweaked according to performance tests.

						float growFactor = 3.0;

						size_t oldSizeBytes = numAllocated * sizeof(T);

						if (oldSizeBytes > 400000u) {
							// Avoid bloat
							growFactor = 1.5f;
						}
						else if (oldSizeBytes > 64000u) {
							// This is what std:: uses at all times
							growFactor = 2.0f;
						}

						numAllocated = (num - numAllocated) + (numAllocated * growFactor);

						if (numAllocated < minSize) {
							numAllocated = minSize;
						}
					}

					realloc(oldNum);
				}

			}
			else if ((num <= numAllocated / 3u) && shrinkIfNecessary && (num > minSize)) {
				// Shrink the underlying array

				// Only copy over old elements that still remain after resizing
				// (destructors were called for others if we're shrinking)
				realloc(iMin(num, oldNum));

			}

			// Call the constructors on newly revealed elements.
			// Do not use parens because we don't want the intializer
			// invoked for POD types.
			for (size_t i = oldNum; i < num; ++i) {
				new (data + i) T;
			}
		}

		/**
		 Add an element to the end of the array.  Will not shrink the underlying array
		 under any circumstances.  It is safe to append an element that is already
		 in the array.
		 */
		inline void append(const T& value) {

			if (num < numAllocated) {
				// This is a simple situation; just stick it in the next free slot using
				// the copy constructor.
				new (data + num) T(value);

				++num;
			}
			else if (inArray(&value)) {
				// The value was in the original array; resizing
				// is dangerous because it may move the value
				// we have a reference to.
				T tmp = value;

				append(tmp);
			}
			else {
				// Here we run the empty initializer where we don't have to, but
				// this simplifies the computation.
				resize(num + 1u, DONT_SHRINK_UNDERLYING_ARRAY);

				data[num - 1u] = value;
			}
		}


		inline void append(const T& v1, const T& v2) {
			if (inArray(&v1) || inArray(&v2)) {
				// Copy into temporaries so that the references won't break when
				// the array resizes.
				T t1 = v1;
				T t2 = v2;

				append(t1, t2);
			}
			else if (num + 1u < numAllocated) {
				// This is a simple situation; just stick it in the next free slot using
				// the copy constructor.
				new (data + num) T(v1);
				new (data + num + 1u) T(v2);

				num += 2u;
			}
			else {
				// Resize the array.  Note that neither value is already in the array.
				resize(num + 2u, DONT_SHRINK_UNDERLYING_ARRAY);

				data[num - 2u] = v1;
				data[num - 1u] = v2;
			}
		}


		inline void append(const T& v1, const T& v2, const T& v3) {
			if (inArray(&v1) || inArray(&v2) || inArray(&v3)) {
				T t1 = v1;
				T t2 = v2;
				T t3 = v3;

				append(t1, t2, t3);
			}
			else if (num + 2u < numAllocated) {
				// This is a simple situation; just stick it in the next free slot using
				// the copy constructor.
				new (data + num) T(v1);
				new (data + num + 1u) T(v2);
				new (data + num + 2u) T(v3);

				num += 3u;
			}
			else {
				resize(num + 3u, DONT_SHRINK_UNDERLYING_ARRAY);

				data[num - 3u] = v1;
				data[num - 2u] = v2;
				data[num - 1u] = v3;
			}
		}


		inline void append(const T& v1, const T& v2, const T& v3, const T& v4) {
			if (inArray(&v1) || inArray(&v2) || inArray(&v3) || inArray(&v4)) {
				T t1 = v1;
				T t2 = v2;
				T t3 = v3;
				T t4 = v4;

				append(t1, t2, t3, t4);
			}
			else if (num + 3u < numAllocated) {
				// This is a simple situation; just stick it in the next free slot using
				// the copy constructor.
				new (data + num) T(v1);
				new (data + num + 1u) T(v2);
				new (data + num + 2u) T(v3);
				new (data + num + 3u) T(v4);

				num += 4u;
			}
			else {
				resize(num + 4u, DONT_SHRINK_UNDERLYING_ARRAY);

				data[num - 4u] = v1;
				data[num - 3u] = v2;
				data[num - 2u] = v3;
				data[num - 1u] = v4;
			}
		}

		/**
		 Returns true if the given element is in the array.
		 */
		bool contains(const T& e) const {
			for (size_t i = 0u; i < size(); ++i) {
				if ((*this)[i] == e) {
					return true;
				}
			}

			return false;
		}

		/**
		 Append the elements of array.  Cannot be called with this array
		 as an argument.
		 */
		void append(const Array<T>& array) {
			debugAssert(this != &array);

			size_t oldNum = num;
			size_t arrayLength = array.length();

			resize(num + arrayLength, false);

			for (size_t i = 0u; i < arrayLength; i++) {
				data[oldNum + i] = array.data[i];
			}
		}

		/**
		 Pushes a new element onto the end and returns its address.
		 This is the same as A.resize(A.size() + 1, false); A.last()
		 */
		inline T& next() {
			resize(num + 1u, false);
			return last();
		}

		/**
		 Pushes an element onto the end (appends)
		 */
		inline void push(const T& value) {
			append(value);
		}

		inline void push(const Array<T>& array) {
			append(array);
		}

		/** Alias to provide std::vector compatibility */
		inline void push_back(const T& v) {
			push(v);
		}

		/** "The member function removes the last element of the controlled sequence, which must be non-empty."
			 For compatibility with std::vector. */
		inline void pop_back() {
			pop();
		}

		/**
		   "The member function returns the storage currently allocated to hold the controlled
			sequence, a value at least as large as size()"
			For compatibility with std::vector.
		*/
		int capacity() const {
			return numAllocated;
		}

		/**
		   "The member function returns a reference to the first element of the controlled sequence,
			which must be non-empty."
			For compatibility with std::vector.
		*/
		T& front() {
			return (*this)[0];
		}

		/**
		   "The member function returns a reference to the first element of the controlled sequence,
			which must be non-empty."
			For compatibility with std::vector.
		*/
		const T& front() const {
			return (*this)[0];
		}

		/**
		   "The member function returns a reference to the last element of the controlled sequence,
			which must be non-empty."
			For compatibility with std::vector.
		*/
		T& back() {
			return (*this)[size() - 1u];
		}

		/**
		   "The member function returns a reference to the last element of the controlled sequence,
			which must be non-empty."
			For compatibility with std::vector.
		*/
		const T& back() const {
			return (*this)[size() - 1u];
		}

		/**
		 Removes the last element and returns it.  By default, shrinks the underlying array.
		 */
		inline T pop(bool shrinkUnderlyingArrayIfNecessary = true) {
			debugAssert(num > 0u);

			T temp = data[num - 1u];

			resize(num - 1u, shrinkUnderlyingArrayIfNecessary);

			return temp;
		}

		/** Pops the last element and discards it without returning anything.  Faster than pop.
		   By default, does not shrink the underlying array.*/
		inline void popDiscard(bool shrinkUnderlyingArrayIfNecessary = false) {
			debugAssert(num > 0u);

			resize(num - 1u, shrinkUnderlyingArrayIfNecessary);
		}


		/**
		 "The member function swaps the controlled sequences between *this and str."
		 Note that this is slower than the optimal std implementation.

		 For compatibility with std::vector.
		 */
		void swap(Array<T>& str) {
			Array<T> temp = str;
			str = *this;
			*this = temp;
		}


		/**
		 Performs bounds checks in debug mode
		 */
		inline T& operator[](uint32_t n) {
			debugAssertM((n >= 0) && (n < num), format("Array index out of bounds. n = %d, size() = %d", n, num));
			debugAssert(data != nullptr);
			return data[n];
		}

		inline T& operator[](int32_t n) {
			debugAssertM(n < num, format("Array index out of bounds. n = %d, size() = %d", n, num));
			return data[n];
		}

		/**
		 Performs bounds checks in debug mode
		 */
		inline const T& operator[](uint32_t n) const {
			debugAssert((n >= 0u) && (n < num));
			debugAssert(data != nullptr);
			return data[n];
		}

		inline const T& operator[](int32_t n) const {
			debugAssert((n < num));
			debugAssert(data != nullptr);
			return data[n];
		}

		inline T& randomElement() {
			debugAssert(num > 0u);
			debugAssert(data != nullptr);
			return data[iRandom(0u, num - 1u)];
		}

		inline const T& randomElement() const {
			debugAssert(num > 0u);
			debugAssert(data != nullptr);
			return data[iRandom(0u, num - 1u)];
		}

		/**
		Returns the last element, performing a check in
		debug mode that there is at least one element.
		*/
		inline const T& last() const {
			debugAssert(num > 0u);
			debugAssert(data != nullptr);
			return data[num - 1u];
		}

		/** Returns element lastIndex() */
		inline T& last() {
			debugAssert(num > 0u);
			debugAssert(data != nullptrr);
			return data[num - 1u];
		}

		/** Returns <i>size() - 1</i> */
		inline size_t lastIndex() const {
			debugAssertM(num > 0u, "Array is empty");
			return num - 1u;
		}

		inline size_t firstIndex() const {
			debugAssertM(num > 0u, "Array is empty");
			return 0u;
		}

		/** Returns element firstIndex(), performing a check in debug mode to ensure that there is at least one */
		inline T& first() {
			debugAssertM(num > 0u, "Array is empty");
			return data[0];
		}

		inline const T& first() const {
			debugAssertM(num > 0u, "Array is empty");
			return data[0];
		}

		/** Returns iFloor(size() / 2), throws an assertion in debug mode if the array is empty */
		inline size_t middleIndex() const {
			debugAssertM(num > 0u, "Array is empty");
			return num >> 1;
		}

		/** Returns element middleIndex() */
		inline const T& middle() const {
			debugAssertM(num > 0u, "Array is empty");
			return data[num >> 1u];
		}

		/** Returns element middleIndex() */
		inline T& middle() {
			debugAssertM(num > 0u, "Array is empty");
			return data[num >> 1u];
		}

		/**
		Calls delete on all objects[0...size-1]
		and sets the size to zero.
		*/
		void deleteAll() {
			for (size_t i = 0u; i < num; i++) {
				delete data[i];
			}
			resize(0u);
		}

		/**
		 Returns the index of (the first occurance of) an index or -1 if
		 not found.  Searches from the right.
		 */
		int32_t rfindIndex(const T& value) const {
			for (size_t i = num - 1u; i >= 0u; --i) {
				if (data[i] == value) {
					return i;
				}
			}

			return -1;
		}

		/**
		 Returns the index of (the first occurance of) an index or -1 if
		 not found.
		 */
		int32_t findIndex(const T& value) const {
			for (size_t i = 0u; i < num; ++i) {
				if (data[i] == value) {
					return i;
				}
			}
			return -1;
		}

		/**
		 Finds an element and returns the iterator to it.  If the element
		 isn't found then returns end().
		 */
		Iterator find(const T& value) {
			for (size_t i = 0u; i < num; ++i) {
				if (data[i] == value) {
					return data + i;
				}
			}
			return end();
		}

		ConstIterator find(const T& value) const {
			for (size_t i = 0u; i < num; ++i) {
				if (data[i] == value) {
					return data + i;
				}
			}
			return end();
		}

		/**
		 Removes count elements from the array
		 referenced either by index or Iterator.
		 */
		void remove(Iterator element, size_t count = 1u) {
			debugAssert((element >= begin()) && (element < end()));
			debugAssert((count > 0u) && (element + count) <= end());
			Iterator last = end() - count;

			while (element < last) {
				element[0] = element[count];
				++element;
			}

			resize(num - count);
		}

		void remove(size_t index, size_t count = 1u) {
			debugAssert((index >= 0u) && (index < num));
			debugAssert((count > 0u) && (index + count <= num));

			remove(begin() + index, count);
		}

		/**
		 Reverse the elements of the array in place.
		 */
		void reverse() {
			T temp;

			size_t n2 = num / 2u;
			for (size_t i = 0u; i < n2; ++i) {
				temp = data[num - 1u - i];
				data[num - 1u - i] = data[i];
				data[i] = temp;
			}
		}

		/**
		 Sort using a specific less-than function, e.g.:

	  <PRE>
		bool CALLSPEC myLT(const MyClass& elem1, const MyClass& elem2) {
			return elem1.x < elem2.x;
		}
		</PRE>

	  Note that for pointer arrays, the <CODE>const</CODE> must come
	  <I>after</I> the class name, e.g., <CODE>Array<MyClass*></CODE> uses:

	  <PRE>
		bool CALLSPEC myLT(MyClass*const& elem1, MyClass*const& elem2) {
			return elem1->x < elem2->x;
		}
		</PRE>

		or a functor, e.g.,
		<pre>
	bool
	less_than_functor::operator()( const double& lhs, const double& rhs ) const
	{
	return( lhs < rhs? true : false );
	}
	</pre>
		 */
		 //    void sort(bool (CALLSPEC *lessThan)(const T& elem1, const T& elem2)) {
		 //    std::sort(data, data + num, lessThan);
		 //}
		template<class LessThan>
		void sort(const LessThan& lessThan) {
			// Using std::sort, which according to http://www.open-std.org/JTC1/SC22/WG21/docs/D_4.cpp
			// was 2x faster than qsort for arrays around size 2000 on intel core2 with gcc
			std::sort(data, data + num, lessThan);
		}

		/**
		 Sorts the array in increasing order using the > or < operator.  To
		 invoke this method on Array<T>, T must override those operator.
		 You can overide these operators as follows:
		 <code>
			bool T::operator>(const T& other) const {
			   return ...;
			}
			bool T::operator<(const T& other) const {
			   return ...;
			}
		 </code>
		 */
		void sort(size_t direction = SORT_INCREASING) {
			if (direction == SORT_INCREASING) {
				std::sort(data, data + num);
			}
			else {
				std::sort(data, data + num, compareGT);
			}
		}

		/**
		 Sorts elements beginIndex through and including endIndex.
		 */
		void sortSubArray(size_t beginIndex, size_t endIndex, size_t direction = SORT_INCREASING) {
			if (direction == SORT_INCREASING) {
				std::sort(data + beginIndex, data + endIndex + 1u);
			}
			else {
				std::sort(data + beginIndex, data + endIndex + 1u, compareGT);
			}
		}

		void sortSubArray(size_t beginIndex, size_t endIndex, bool (CALLSPEC* lessThan)(const T& elem1, const T& elem2)) {
			std::sort(data + beginIndex, data + endIndex + 1u, lessThan);
		}

		/**
		 The StrictWeakOrdering can be either a class that overloads the function call operator() or
		 a function pointer of the form <code>bool (CALLSPEC *lessThan)(const T& elem1, const T& elem2)</code>
		 */
		template<typename StrictWeakOrdering>
		void sortSubArray(size_t beginIndex, size_t endIndex, StrictWeakOrdering& lessThan) {
			std::sort(data + beginIndex, data + endIndex + 1u, lessThan);
		}

		/** Uses < and == to evaluate operator(); this is the default comparator for Array::partition. */
		class DefaultComparator {
		public:
			inline int8_t operator()(const T& A, const T& B) const {
				if (A < B) {
					return 1;
				}
				else if (A == B) {
					return 0;
				}
				else {
					return -1;
				}
			}
		};

		/** The output arrays are resized with fastClear() so that if they are already of the same size
			as this array no memory is allocated during partitioning.

			@param comparator A function, or class instance with an overloaded operator() that compares
			two elements of type <code>T</code> and returns 0 if they are equal, -1 if the second is smaller,
			and 1 if the first is smaller (i.e., following the conventions of std::string::compare).  For example:

			<pre>
			int compare(int A, int B) {
				if (A < B) {
					return 1;
				} else if (A == B) {
					return 0;
				} else {
					return -1;
				}
			}
			</pre>
			*/
		template<typename Comparator>
		void partition(
			const T& partitionElement,
			Array<T>& ltArray,
			Array<T>& eqArray,
			Array<T>& gtArray,
			const Comparator& comparator) const {

			// Make sure all arrays are independent
			debugAssert(&ltArray != this);
			debugAssert(&eqArray != this);
			debugAssert(&gtArray != this);
			debugAssert(&ltArray != &eqArray);
			debugAssert(&ltArray != &gtArray);
			debugAssert(&eqArray != &gtArray);

			// Clear the arrays
			ltArray.fastClear();
			eqArray.fastClear();
			gtArray.fastClear();

			// Form a table of buckets for lt, eq, and gt
			Array<T>* bucket[3] = { &ltArray, &eqArray, &gtArray };

			for (size_t i = 0u; i < num; ++i) {
				int8_t c = comparator(partitionElement, data[i]);
				debugAssertM(c >= -1 && c <= 1, "Comparator returned an illegal value.");

				// Insert into the correct bucket, 0, 1, or 2
				bucket[c + 1]->append(data[i]);
			}
		}

		/**
		  Uses < and == on elements to perform a partition.  See partition().
		 */
		void partition(
			const T& partitionElement,
			Array<T>& ltArray,
			Array<T>& eqArray,
			Array<T>& gtArray) const {

			partition(partitionElement, ltArray, eqArray, gtArray, typename Array<T>::DefaultComparator());
		}

		/**
		 Paritions the array into those below the median, those above the median, and those elements
		 equal to the median in expected O(n) time using quickselect.  If the array has an even
		 number of different elements, the median for partition purposes is the largest value
		 less than the median.

		 @param tempArray used for working scratch space
		 @param comparator see parition() for a discussion.*/
		template<typename Comparator>
		void medianPartition(
			Array<T>& ltMedian,
			Array<T>& eqMedian,
			Array<T>& gtMedian,
			Array<T>& tempArray,
			const Comparator& comparator) const {

			ltMedian.fastClear();
			eqMedian.fastClear();
			gtMedian.fastClear();

			// Handle trivial cases first
			switch (size()) {
			case 0u:
				// Array is empty; no parition is possible
				return;

			case 1u:
				// One element
				eqMedian.append(first());
				return;

			case 2u: {
				// Two element array; median is the smaller
				int8_t c = comparator(first(), last());

				switch (c) {
				case -1:
					// first was bigger
					eqMedian.append(last());
					gtMedian.append(first());
					break;

				case 0:
					// Both equal to the median
					eqMedian.append(first(), last());
					break;

				case 1:
					// Last was bigger
					eqMedian.append(first());
					gtMedian.append(last());
					break;
				}
			}
			return;
			}

			// All other cases use a recursive randomized median

			// Number of values less than all in the current arrays        
			size_t ltBoost = 0u;

			// Number of values greater than all in the current arrays        
			size_t gtBoost = 0u;

			// For even length arrays, force the gt array to be one larger than the
			// lt array:  
			//  [1 2 3] size = 3, choose half = (s + 1) /2
			//
			size_t lowerHalfSize, upperHalfSize;
			if (isEven(size())) {
				lowerHalfSize = size() / 2u;
				upperHalfSize = lowerHalfSize + 1u;
			}
			else {
				lowerHalfSize = upperHalfSize = (size() + 1u) / 2u;
			}
			const T* xPtr = nullptr;

			// Maintain pointers to the arrays; we'll switch these around during sorting
			// to avoid copies.
			const Array<T>* source = this;
			Array<T>* lt = &ltMedian;
			Array<T>* eq = &eqMedian;
			Array<T>* gt = &gtMedian;
			Array<T>* extra = &tempArray;

			while (true) {
				// Choose a random element -- choose the middle element; this is theoretically
				// suboptimal, but for loosly sorted array is actually the best strategy

				xPtr = &(source->middle());
				if (source->size() == 1u) {
					// Done; there's only one element left
					break;
				}
				const T& x = *xPtr;

				// Note: partition (fast) clears the arrays for us
				source->partition(x, *lt, *eq, *gt, comparator);

				size_t L = lt->size() + ltBoost + eq->size();
				size_t U = gt->size() + gtBoost + eq->size();
				if ((L >= lowerHalfSize) &&
					(U >= upperHalfSize)) {

					// x must be the partition median                    
					break;

				}
				else if (L < lowerHalfSize) {

					// x must be smaller than the median.  Recurse into the 'gt' array.
					ltBoost += lt->size() + eq->size();

					// The new gt array will be the old source array, unless
					// that was the this pointer (i.e., unless we are on the 
					// first iteration)
					Array<T>* newGt = (source == this) ? extra : const_cast<Array<T>*>(source);

					// Now set up the gt array as the new source
					source = gt;
					gt = newGt;

				}
				else {

					// x must be bigger than the median.  Recurse into the 'lt' array.
					gtBoost += gt->size() + eq->size();

					// The new lt array will be the old source array, unless
					// that was the this pointer (i.e., unless we are on the 
					// first iteration)
					Array<T>* newLt = (source == this) ? extra : const_cast<Array<T>*>(source);

					// Now set up the lt array as the new source
					source = lt;
					lt = newLt;
				}
			}

			// Now that we know the median, make a copy of it (since we're about to destroy the array that it
			// points into).
			T median = *xPtr;
			xPtr = nullptr;

			// Partition the original array (note that this fast clears for us)
			partition(median, ltMedian, eqMedian, gtMedian, comparator);
		}

		/**
		  Computes a median partition using the default comparator and a dynamically allocated temporary
		  working array.  If the median is not in the array, it is chosen to be the largest value smaller
		  than the true median.
		 */
		void medianPartition(
			Array<T>& ltMedian,
			Array<T>& eqMedian,
			Array<T>& gtMedian) const {

			Array<T> temp;
			medianPartition(ltMedian, eqMedian, gtMedian, temp, DefaultComparator());
		}


		/** Redistributes the elements so that the new order is statistically independent
			of the original order. O(n) time.*/
		void randomize() {
			T temp;

			for (size_t i = size() - 1u; i >= 0u; --i) {
				size_t x = Random::integer(0u, i);
				
				temp = data[i];
				data[i] = data[x];
				data[x] = temp;
			}
		}


	};


	/** Array::contains for C-arrays */
	template<class T> bool contains(const T* array, size_t len, const T& e) {
		for (size_t i = len - 1u; i >= 0u; --i) {
			if (array[i] == e) {
				return true;
			}
		}

		return false;
	}

} // namespace

#ifdef _MSC_VER
#   pragma warning (pop)
#endif

#endif
