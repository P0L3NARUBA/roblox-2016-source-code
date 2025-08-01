/**
  @file Color4uint8.h

  @maintainer Morgan McGuire, http://graphics.cs.williams.edu

  @created 2003-04-07
  @edited  2010-03-24

  Copyright 2000-2010, Morgan McGuire.
  All rights reserved.
 */

#ifndef COLOR4UINT8_H
#define COLOR4UINT8_H

#include "G3D/g3dmath.h"
#include "G3D/platform.h"
#include "G3D/Color3uint8.h"

namespace G3D {

	/**
	 Represents a Color4 as a packed integer.  Convenient
	 for creating unsigned int vertex arrays.  Used by
	 G3D::GImage as the underlying format.

	 <B>WARNING</B>: Integer color formats are different than
	 integer vertex formats.  The color channels are automatically
	 scaled by 255 (because OpenGL automatically scales integer
	 colors back by this factor).  So Color4(1,1,1) == Color4uint8(255,255,255)
	 but Vector3(1,1,1) == Vector3int16(1,1,1).

	 */
	G3D_BEGIN_PACKED_CLASS(1)
		class Color4uint8 {
		private:
			// Hidden operators
			bool operator<(const Color4uint8&) const;
			bool operator>(const Color4uint8&) const;
			bool operator<=(const Color4uint8&) const;
			bool operator>=(const Color4uint8&) const;

		public:
			uint8_t r;
			uint8_t g;
			uint8_t b;
			uint8_t a;

			Color4uint8() : r(0u), g(0u), b(0u), a(0u) {}

			Color4uint8(const class Color4& c);

			Color4uint8 max(const Color4uint8 x) const {
				return Color4uint8(G3D::max(r, x.r), G3D::max(g, x.g), G3D::max(b, x.b), G3D::max(a, x.a));
			}

			Color4uint8 min(const Color4uint8 x) const {
				return Color4uint8(G3D::min(r, x.r), G3D::min(g, x.g), G3D::min(b, x.b), G3D::min(a, x.a));
			}

			Color4uint8(const uint8_t _r, const uint8_t _g, const uint8_t _b, const uint8_t _a) : r(_r), g(_g), b(_b), a(_a) {}

			Color4uint8(const Color3uint8& c, const uint8_t _a) : r(c.r), g(c.g), b(c.b), a(_a) {}

			inline static Color4uint8 fromARGB(uint32_t i) {
				Color4uint8 c;
				c.a = (i >> 24) & 0xFF;
				c.r = (i >> 16) & 0xFF;
				c.g = (i >> 8) & 0xFF;
				c.b = i & 0xFF;
				return c;
			}

			inline uint32_t asUInt32() const {
				return ((uint32_t)a << 24u) + ((uint32_t)r << 16u) + ((uint32_t)g << 8u) + b;
			}

			// access vector V as V[0] = V.r, V[1] = V.g, V[2] = V.b
			//
			// WARNING.  These member functions rely on
			// (1) Color4uint8 not having virtual functions
			// (2) the data packed in a 3*sizeof(uint8) memory block
			uint8_t& operator[] (size_t i) const {
				return ((uint8_t*)this)[i];
			}

			uint8_t& operator[] (int8_t i) const {
				return ((uint8_t*)this)[i];
			}

			uint8_t& operator[] (int16_t i) const {
				return ((uint8_t*)this)[i];
			}

			uint8_t& operator[] (int32_t i) const {
				return ((uint8_t*)this)[i];
			}

			operator uint8_t* () {
				return (uint8_t*)this;
			}

			operator const uint8_t* () const {
				return (uint8_t*)this;
			}


			inline Color3uint8 bgr() const {
				return Color3uint8(b, g, r);
			}

			inline Color3uint8 rgb() const {
				return Color3uint8(r, g, b);
			}

			bool operator==(const Color4uint8& other) const {
				return *reinterpret_cast<const uint32_t*>(this) == *reinterpret_cast<const uint32_t*>(&other);
			}

			bool operator!=(const Color4uint8& other) const {
				return *reinterpret_cast<const uint32_t*>(this) != *reinterpret_cast<const uint32_t*>(&other);
			}

	}
	G3D_END_PACKED_CLASS(1)

} // namespace G3D

#endif
