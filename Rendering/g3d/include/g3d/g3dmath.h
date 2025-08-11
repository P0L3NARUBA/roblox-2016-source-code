/**
 @file g3dmath.h

 Math util class.

 @maintainer Morgan McGuire, http://graphics.cs.williams.edu
 @cite highestBit by Jukka Liimatta

 @created 2001-06-02
 @edited  2009-04-07

 Copyright 2000-2006, Morgan McGuire.
 All rights reserved.
 */

#ifndef G3D_g3dmath_h
#define G3D_g3dmath_h

#ifdef _MSC_VER
 // Disable conditional expression is constant, which occurs incorrectly on inlined functions
#   pragma warning (push)
#   pragma warning (disable : 4127)
// disable: "C++ exception handler used"
#   pragma warning (disable : 4530)
#endif

#include "G3D/platform.h"
#include <ctype.h>
#include <float.h>
#include <limits>
#include <stdlib.h>
#include <cstdint>

#ifdef _MSC_VER
	// Visual Studio is missing inttypes.h
#   ifndef PRId64
#       define PRId64 "I64d"
#   endif
#else
#include <inttypes.h>
#endif

/*These defines enable functionality introduced with the 1999 ISO C
**standard. They must be defined before the inclusion of math.h to
**engage them. If optimisation is enabled, these functions will be
**inlined. With optimisation switched off, you have to link in the
**maths library using -lm.
*/

#define _ISOC9X_SOURCE1
#define _ISOC99_SOURCE1
#define __USE_ISOC9X1
#define __USE_ISOC991

#include <math.h>

#include "G3D/debug.h"

#undef min
#undef max

namespace G3D {



#define fuzzyEpsilon (0.00001f)
	/**
		This value should not be tested against directly, instead
		G3D::isNan() and G3D::isFinite() will return reliable results. */
	double inf();

	/** This value should not be tested against directly, instead
		G3D::isNan() and G3D::isFinite() will return reliable results. */
	double nan();

	float finf();

	float fnan();

	inline double pi() {
		return 3.1415926535898;
	}

	inline float pif() {
		return 3.1415926535898f;
	}

	inline double halfPi() {
		return 1.57079633;
	}

	inline double twoPi() {
		return 6.28318531;
	}


	typedef signed char     int8;	// @deprecated use int8_t instead
	typedef unsigned char	uint8;	// @deprecated use uint8_t instead
	typedef short           int16;	// @deprecated use int16_t instead
	typedef unsigned short  uint16;	// @deprecated use uint16_t instead
	typedef int             int32;	// @deprecated use int32_t instead
	typedef unsigned int    uint32;	// @deprecated use uint32_t instead

#ifdef _MSC_EXTENSIONS
	typedef __int64             int64;	// @deprecated use int64_t instead
	typedef unsigned __int64    uint64;	// @deprecated use uint64_t instead
#elif ! defined(_MSC_VER)
	typedef int64_t             int64;	// @deprecated use int64_t instead
	typedef uint64_t            uint64;	// @deprecated use uint64_t instead
#else
	typedef long long           int64;	// @deprecated use int64_t instead
	typedef unsigned long long  uint64;	// @deprecated use uint64_t instead
#endif

	typedef float  float32; // @deprecated use float instead
	typedef double float64; // @deprecated use double instead

	int32_t iAbs(int32_t iValue);
	int32_t iCeil(double fValue);

	/**
	 Clamps the value to the range [low, hi] (inclusive)
	 */
	int16_t iClamp(int16_t val, int16_t low, int16_t hi);
	int32_t iClamp(int32_t val, int32_t low, int32_t hi);
	double clamp(double val, double low, double hi);
	float clamp(float val, float low, float hi);

	/**
	 Returns a + (b - a) * f;
	 */
	inline double lerp(double a, double b, double f) {
		return a + (b - a) * f;
	}

	inline float lerp(float a, float b, float f) {
		return a + (b - a) * f;
	}

	/**
	 Wraps the value to the range [0, hi) (exclusive
	 on the high end).  This is like the clock arithmetic
	 produced by % (modulo) except the result is guaranteed
	 to be positive.
	 */
	int32_t iWrap(int32_t val, int32_t hi);

	int32_t iFloor(double fValue);

	int32_t iSign(int32_t iValue);
	int32_t iSign(double fValue);

	inline int32_t iSign(float f) {
		return iSign((double)f);
	}


	/**
		Fast round to integer using the lrint routine.
		Typically 6x faster than casting to integer.
	 */
	inline int32_t iRound(double fValue) {
		return (fValue >= 0) ? int32_t(fValue + 0.5) : int32_t(fValue - 0.5);
	}

	/**
		Fast round to integer using the lrint routine.
		Typically 6x faster than casting to integer.
	 */
	inline int32_t iRound(float fValue) {
		return (fValue >= 0) ? int32_t(fValue + 0.5f) : int32_t(fValue - 0.5f);
	}

	/**
	 Returns a random number uniformly at random between low and hi
	 (inclusive).
	 @deprecated Use Random::integer
	 */
	int32_t iRandom(int32_t low, int32_t hi);

	double abs(double fValue);
	double aCos(double fValue);
	double aSin(double fValue);
	double aTan(double fValue);
	double aTan2(double fY, double fX);
	double sign(double fValue);
	double square(double fValue);

	/**
	 Returns true if the argument is a finite real number.
	 */
	bool isFinite(double x);

	/**
	 Returns true if the argument is NaN (not a number).
	 You can't use x == nan to test this because all
	 comparisons against nan return false.
	 */
	bool isNaN(double x);
	bool isNaN(float x);
	inline bool isNaN(int32_t x) {
		(void)x;
		return false;
	}

	/**
	 Computes x % 3.
	 */
	int32_t iMod3(int32_t x);

	/**
	 Uniform random number between low and hi, inclusive. [low, hi]
	 @deprecated
	 @sa Random::uniform
	 */
	float uniformRandom(float low = 0.0f, float hi = 1.0f);

	// ROBLOX
	inline int32_t uniformRandomInt(int32_t low, int32_t hi) {
		int32_t answer = static_cast<int32_t>(uniformRandom(static_cast<float>(low), static_cast<float>(hi)));
		return iClamp(answer, low, hi);
	}
	// ========


	/**
	 Normally distributed random number.

	 @deprecated
	 @sa Random::gaussian
	 */
	float gaussRandom(float mean = 0.0f, float stdev = 1.0f);


	/** Returns x<sup>5</sup> */
	template <class T>
	inline T pow5(T x) {
		const T y = x * x;
		return y * y * x;
	}


	template <class T>
	inline T min(const T& x, const T& y) {
		return std::min<T>(x, y);
	}

	template <class T>
	inline T min(const T& x, const T& y, const T& z) {
		return std::min<T>(std::min<T>(x, y), z);
	}

	template <class T>
	inline T min(const T& x, const T& y, const T& z, const T& w) {
		return std::min<T>(std::min<T>(x, y), std::min<T>(z, w));
	}

	template <class T>
	inline T max(const T& x, const T& y) {
		return std::max<T>(x, y);
	}

	template <class T>
	inline T max(const T& x, const T& y, const T& z) {
		return std::max<T>(std::max<T>(x, y), z);
	}

	template <class T>
	inline T max(const T& x, const T& y, const T& z, const T& w) {
		return std::max<T>(std::max<T>(x, y), std::max<T>(z, w));
	}

	int32_t iMin(int32_t x, int32_t y);
	int32_t iMax(int32_t x, int32_t y);

	double square(double x);
	double sumSquares(double x, double y);
	double sumSquares(double x, double y, double z);
	double distance(double x, double y);
	double distance(double x, double y, double z);

	/**
	  Returnes the 0-based index of the highest 1 bit from
	  the left.  -1 means the number was 0.

	  @cite Based on code by jukka@liimatta.org
	 */
	int32_t highestBit(uint32_t x);

	/**
	 Note that fuzzyEq(a, b) && fuzzyEq(b, c) does not imply
	 fuzzyEq(a, c), although that will be the case on some
	 occasions.
	 */
	bool fuzzyEq(double a, double b);

	/** True if a is definitely not equal to b.
		Guaranteed false if a == b.
		Possibly false when a != b.*/
	bool fuzzyNe(double a, double b);

	/** Is a strictly greater than b? (Guaranteed false if a <= b).
		(Possibly false if a > b) */
	bool fuzzyGt(double a, double b);

	/** Is a near or greater than b? */
	bool fuzzyGe(double a, double b);

	/** Is a strictly less than b? (Guaranteed false if a >= b)*/
	bool fuzzyLt(double a, double b);

	/** Is a near or less than b? */
	bool fuzzyLe(double a, double b);

	/**
	 Computes 1 / sqrt(x).
	 */
	inline float rsq(float x) {
		return 1.0f / sqrtf(x);
	}

	/**
	 Return the next power of 2 higher than the input
	 If the input is already a power of 2, the output will be the same
	 as the input.
	 */
	uint32_t ceilPow2(uint32_t in);

	/** Returns 2^x */
	inline uint32_t pow2(uint32_t x) {
		return 1 << x;
	}

	inline double log2(double x) {
		return ::log(x) * 1.442695;
	}

	inline float log2(float x) {
		return ::logf(x) * 1.442695f;
	}

	inline double log2(int32_t x) {
		return log2((double)x);
	}

	inline double log2(uint32_t x) {
		return log2((double)x);
	}

	/**
	 * True if num is a power of two.
	 */
	bool isPow2(int32_t num);

	bool isOdd(int32_t num);
	bool isEven(int32_t num);

	double toRadians(double deg);
	double toDegrees(double rad);

	/**
	 Returns true if x is not exactly equal to 0.0f.
	 */
	inline bool any(float x) {
		return x != 0.0f;
	}

	/**
	 Returns true if x is not exactly equal to 0.0f.
	 */
	inline bool all(float x) {
		return x != 0.0f;
	}

	/**
	 v / v (for DirectX/Cg support)
	 */
	inline float normalize(float v) {
		return v / v;
	}

	/**
	 a * b (for DirectX/Cg support)
	 */
	inline float dot(float a, float b) {
		return a * b;
	}


	/**
	 a * b (for DirectX/Cg support)
	 */
	inline float mul(float a, float b) {
		return a * b;
	}

	/**
	 2^x
	 */
	inline double exp2(double x) {
		return pow(2.0, x);
	}

	inline float exp2(float x) {
		return powf(2.0f, x);
	}

	/** @deprecated Use rsq */
	inline double rsqrt(double x) {
		return 1.0 / sqrt(x);
	}

	/** @deprecated Use rsq */
	inline float rsqrt(float x) {
		// TODO: default this to using the SSE2 instruction
		return 1.0f / sqrtf(x);
	}

	/**
	 sin(x)/x
	 */
	inline double sinc(double x) {
		double r = sin(x) / x;

		if (isNaN(r))
			return 1.0;
		else
			return r;
	}

	/**
	 Computes a floating point modulo; the result is t wrapped to the range [lo, hi).
	 */
	inline float wrap(float t, float lo, float hi) {
		if ((t >= lo) && (t < hi)) {
			return t;
		}

		debugAssert(hi > lo);

		float interval = hi - lo;

		return t - interval * iFloor((t - lo) / interval);
	}


	inline double wrap(double t, double lo, double hi) {
		if ((t >= lo) && (t < hi)) {
			return t;
		}

		debugAssert(hi > lo);

		double interval = hi - lo;

		return t - interval * iFloor((t - lo) / interval);
	}

	inline double wrap(double t, double hi) {
		return wrap(t, 0.0, hi);
	}


	inline bool isFinite(double x) {
		return !isNaN(x) && (x < G3D::inf()) && (x > -G3D::inf());
	}

	inline bool isFinite(float x) {
		return !isNaN(x) && (x < G3D::finf()) && (x > -G3D::finf());
	}

	//----------------------------------------------------------------------------
	inline int32_t iAbs(int32_t iValue) {
		return (iValue >= 0 ? iValue : -iValue);
	}

	//----------------------------------------------------------------------------
	inline int32_t iCeil(double fValue) {
		return int32_t(::ceil(fValue));
	}

	//----------------------------------------------------------------------------

	inline int16_t iClamp(int16_t val, int16_t low, int16_t hi) {
		debugAssert(low <= hi);
		if (val <= low) {
			return low;
		}
		else if (val >= hi) {
			return hi;
		}
		else {
			return val;
		}
	}

	//----------------------------------------------------------------------------

	inline int32_t iClamp(int32_t val, int32_t low, int32_t hi) {
		debugAssert(low <= hi);
		if (val <= low) {
			return low;
		}
		else if (val >= hi) {
			return hi;
		}
		else {
			return val;
		}
	}

	//----------------------------------------------------------------------------

	inline double clamp(double val, double low, double hi) {
		debugAssert(low <= hi);
		if (val <= low) {
			return low;
		}
		else if (val >= hi) {
			return hi;
		}
		else {
			return val;
		}
	}

	inline float clamp(float val, float low, float hi) {
		debugAssert(low <= hi);
		if (val <= low) {
			return low;
		}
		else if (val >= hi) {
			return hi;
		}
		else {
			return val;
		}
	}
	//----------------------------------------------------------------------------

	inline int32_t iWrap(int32_t val, int32_t hi) {
		if (val < 0) {
			return ((val % hi) + hi) % hi;
		}
		else {
			return val % hi;
		}
	}

	//----------------------------------------------------------------------------
	inline int32_t iFloor(double fValue) {
		return int32_t(::floor(fValue));
	}

	//----------------------------------------------------------------------------
	inline int32_t iSign(int32_t iValue) {
		return (iValue > 0 ? +1 : (iValue < 0 ? -1 : 0));
	}

	inline int32_t iSign(double fValue) {
		return (fValue > 0.0 ? +1 : (fValue < 0.0 ? -1 : 0));
	}

	//----------------------------------------------------------------------------
	inline double abs(double fValue) {
		return double(::fabs(fValue));
	}

	//----------------------------------------------------------------------------
	inline double aCos(double fValue) {
		if (-1.0 < fValue) {
			if (fValue < 1.0)
				return double(::acos(fValue));
			else
				return 0.0;
		}
		else {
			return pi();
		}
	}

	inline float acos(float fValue) {
		if (-1.0f < fValue) {
			if (fValue < 1.0f) {
				return ::acos(fValue);
			}
			else {
				return 0.0f;
			}
		}
		else {
			return pif();
		}
	}

	//----------------------------------------------------------------------------
	inline double aSin(double fValue) {
		if (-1.0 < fValue) {
			if (fValue < 1.0) {
				return double(::asin(fValue));
			}
			else {
				return -halfPi();
			}
		}
		else {
			return halfPi();
		}
	}

	//----------------------------------------------------------------------------
	inline double aTan(double fValue) {
		return double(::atan(fValue));
	}

	//----------------------------------------------------------------------------
	inline double aTan2(double fY, double fX) {
		return double(::atan2(fY, fX));
	}

	//----------------------------------------------------------------------------
	inline double sign(double fValue) {
		if (fValue > 0.0) {
			return 1.0;
		}

		if (fValue < 0.0) {
			return -1.0;
		}

		return 0.0;
	}

	inline float sign(float fValue) {
		if (fValue > 0.0f) {
			return 1.0f;
		}

		if (fValue < 0.0f) {
			return -1.0f;
		}

		return 0.0f;
	}


	inline float uniformRandom(float low, float hi) {
		return (hi - low) * float(::rand()) / float(RAND_MAX) + low;
	}

	inline double square(double x) {
		return x * x;
	}

	inline float square(float x) {
		return x * x;
	}

	inline int32_t square(int32_t x) {
		return x * x;
	}

	//----------------------------------------------------------------------------
	inline double sumSquares(double x, double y) {
		return x * x + y * y;
	}

	//----------------------------------------------------------------------------
	inline float sumSquares(float x, float y) {
		return x * x + y * y;
	}

	//----------------------------------------------------------------------------
	inline double sumSquares(double x, double y, double z) {
		return x * x + y * y + z * z;
	}

	//----------------------------------------------------------------------------
	inline float sumSquares(float x, float y, float z) {
		return x * x + y * y + z * z;
	}

	//----------------------------------------------------------------------------
	inline double distance(double x, double y) {
		return sqrt(sumSquares(x, y));
	}

	//----------------------------------------------------------------------------
	inline float distance(float x, float y) {
		return sqrt(sumSquares(x, y));
	}

	//----------------------------------------------------------------------------
	inline double distance(double x, double y, double z) {
		return sqrt(sumSquares(x, y, z));
	}

	//----------------------------------------------------------------------------
	inline float distance(float x, float y, float z) {
		return sqrt(sumSquares(x, y, z));
	}

	//----------------------------------------------------------------------------

	/** @deprecated use G3D::min */
	inline int32_t iMin(int32_t x, int32_t y) {
		return (x >= y) ? y : x;
	}

	//----------------------------------------------------------------------------
	/** @deprecated use G3D::min */
	inline int32_t iMax(int32_t x, int32_t y) {
		return (x >= y) ? x : y;
	}

	//----------------------------------------------------------------------------
	inline uint32_t ceilPow2(uint32_t in) {
		in -= 1u;

		in |= in >> 16u;
		in |= in >> 8u;
		in |= in >> 4u;
		in |= in >> 2u;
		in |= in >> 1u;

		return in + 1u;
	}

	inline bool isPow2(int32_t num) {
		return ((num & -num) == num);
	}

	inline bool isOdd(int32_t num) {
		return (num & 1) == 1;
	}

	inline bool isEven(int32_t num) {
		return (num & 1) == 0;
	}

	inline double toRadians(double deg) {
		return deg * pi() / 180.0;
	}

	inline double toDegrees(double rad) {
		return rad * 180.0 / pi();
	}

	inline float toRadians(float deg) {
		return deg * (float)pi() / 180.0f;
	}

	inline float toDegrees(float rad) {
		return rad * 180.0f / (float)pi();
	}

	inline float toRadians(int32_t deg) {
		return deg * (float)pi() / 180.0f;
	}

	inline float toDegrees(int32_t rad) {
		return rad * 180.0f / (float)pi();
	}

	// ROBLOX
	/**
	 Computes an relative epsilon for comparing a and b, given an absolute epsilon.
	 */
	inline double eps(double a, double b, double absepsilon) {
		// For a and b to be nearly equal, they must have nearly
		// the same magnitude.  This means that we can ignore b
		// since it either has the same magnitude or the comparison
		// will fail anyway.
		(void)b;
		const double aa = abs(a) + 1.0;
		if (aa == inf()) {
			return absepsilon;
		}
		else {
			return absepsilon * aa;
		}
	}
	/**
	 Computes an default relative epsilon for comparing a and b.
	 */
	inline double eps(double a, double b) {
		return eps(a, b, fuzzyEpsilon);
	}

	inline bool fuzzyEq(double a, double b, double absepsilon) {
		return (a == b) || (abs(a - b) <= eps(a, b, absepsilon));
	}

	// ======

	inline bool fuzzyEq(double a, double b) {
		return (a == b) || (abs(a - b) <= eps(a, b));
	}

	inline bool fuzzyNe(double a, double b) {
		return !fuzzyEq(a, b);
	}

	inline bool fuzzyGt(double a, double b) {
		return a > b + eps(a, b);
	}

	inline bool fuzzyGe(double a, double b) {
		return a > b - eps(a, b);
	}

	inline bool fuzzyLt(double a, double b) {
		return a < b - eps(a, b);
	}

	inline bool fuzzyLe(double a, double b) {
		return a < b + eps(a, b);
	}

	inline int iMod3(int x) {
		return x % 3;
	}

	/**
	 Given a 32-bit integer, returns the integer with the bytes in the opposite order.
	 */
	inline uint32_t flipEndian32(const uint32_t x) {
		return (x << 24u) | ((x & 0xFF00u) << 8u) |
			((x & 0xFF0000u) >> 8u) | ((x & 0xFF000000u) >> 24u);
	}

	/**
	 Given a 16-bit integer, returns the integer with the bytes in the opposite order.
	 */
	inline uint16_t flipEndian16(const uint16_t x) {
		return (x << 8u) | ((x & 0xFF00u) >> 8u);
	}


} // namespace

#ifdef _MSC_VER
#   pragma warning (pop)
#endif

#endif

