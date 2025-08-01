#pragma once

#include "Resource.h"

namespace RBX {
	namespace Graphics {

		class Renderbuffer;

		struct TextureRegion {
			uint32_t x;
			uint32_t y;
			uint32_t z;

			uint32_t width;
			uint32_t height;
			uint32_t depth;

			TextureRegion();
			TextureRegion(uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height, uint32_t depth);
			TextureRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
		};

		class Texture : public Resource {
		public:
			enum Type {
				Type_1D,
				Type_1D_Array,
				Type_2D,
				Type_2D_Array,
				Type_3D,
				Type_Cube,
				Type_Cube_Array,

				Type_Count
			};

			enum Usage {
				Usage_Static,		/* CPU: -/- | GPU: R/W */
				Usage_Dynamic,		/* CPU: -/- | GPU: R/W */
				Usage_Immutable,	/* CPU: -/- | GPU: R/- */
				Usage_Renderbuffer,	/* CPU: -/- | GPU: R/W */

				Usage_Count
			};

			enum Format {
				Format_R8,			/* R8  --- --- ---         */
				Format_RG8,			/* R8  G8  --- ---         */
				Format_RGBA8,		/* R8  G8  B8  A8          */
				Format_BGR5A1,		/* B5  G5  R5  A1          */
				Format_R11G11B10f,	/* R11 G11 B10 --- | float */
				Format_RG16,		/* R16 G16 --- ---         */
				Format_RG16f,		/* R16 G16 --- --- | float */
				Format_RGBA16f,		/* R16 G16 B16 A16 | float */

				Format_BC1,			/* R5  G6  B5  A1          */
				Format_BC1_sRGB,	/* R5  G6  B5  A1  | sRGB  */
				Format_BC2,			/* R5  G6  B5  A4          */
				Format_BC2_sRGB,	/* R5  G6  B5  A4  | sRGB  */
				Format_BC3,			/* R5  G6  B5  A8          */
				Format_BC3_sRGB,	/* R5  G6  B5  A8  | sRGB  */
				Format_BC4,			/* R8  --- --- ---         */
				Format_BC5,			/* R8  G8  --- ---         */
				Format_BC6f,		/* R16 G16 B16 ---         */
				Format_BC7,			/* R-- G-- B-- A--         */
				Format_BC7_sRGB,	/* R-- G-- B-- A-- | sRGB  */

				Format_D16,			/* D16 --- --- ---         */
				Format_D24S8,		/* D24 S8  --- ---         */
				Format_D32f,		/* D32 --- --- --- | float */
				Format_D32fS8X24,	/* D32 S8  X24 --- | float */

				Format_Count
			};

			struct LockResult {
				void* data;
				uint32_t rowPitch;
				uint32_t slicePitch;
			};

			~Texture();

			virtual void upload(uint32_t index, uint32_t mip, const TextureRegion& region, const void* data, size_t size) = 0;

			virtual bool download(uint32_t index, uint32_t mip, void* data, size_t size) = 0;

			virtual bool supportsLocking() const = 0;
			virtual LockResult lock(uint32_t index, uint32_t mip, const TextureRegion& region) = 0;
			virtual void unlock(uint32_t index, uint32_t mip) = 0;

			virtual shared_ptr<Renderbuffer> getRenderbuffer(uint32_t index, uint32_t mip) = 0;

			virtual void commitChanges() = 0;
			virtual void generateMipmaps() = 0;

			Type getType() const { return type; }
			Format getFormat() const { return format; }

			uint32_t getWidth() const { return width; }
			uint32_t getHeight() const { return height; }
			uint32_t getDepth() const { return depth; }

			uint32_t getMipLevels() const { return mipLevels; }

			Usage getUsage() const { return usage; }

			static bool isFormatCompressed(Format format);
			static bool isFormatDepth(Format format);
			static uint32_t getFormatBits(Format format);
			static uint32_t getImageSize(Format format, uint32_t width, uint32_t height);

			static uint32_t getTextureSize(Type type, Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels);

			static uint32_t getMipSide(uint32_t value, uint32_t mip);
			static uint32_t getMaxMipCount(uint32_t width, uint32_t height, uint32_t depth);

		protected:
			Texture(Device* device, Type type, Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, Usage usage);

			Type type;
			Format format;
			uint32_t width;
			uint32_t height;
			uint32_t depth;
			uint32_t mipLevels;
			Usage usage;
		};

	}
}
