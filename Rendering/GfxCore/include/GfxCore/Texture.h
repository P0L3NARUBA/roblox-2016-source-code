#pragma once

#include "Resource.h"

namespace RBX
{
	namespace Graphics
	{

		class Renderbuffer;

		struct TextureRegion
		{
			unsigned int x;
			unsigned int y;
			unsigned int z;

			unsigned int width;
			unsigned int height;
			unsigned int depth;

			TextureRegion();
			TextureRegion(unsigned int x, unsigned int y, unsigned int z, unsigned int width, unsigned int height, unsigned int depth);
			TextureRegion(unsigned int x, unsigned int y, unsigned int width, unsigned int height);
		};

		class Texture : public Resource
		{
		public:
			enum Type
			{
				Type_1D,
				Type_1D_Array,
				Type_2D,
				Type_2D_Array,
				Type_3D,
				Type_Cube,
				Type_Cube_Array,

				Type_Count
			};

			enum Usage
			{
				Usage_Static,		/* CPU: -/- | GPU: R/W */
				Usage_Dynamic,		/* CPU: -/- | GPU: R/W */
				Usage_Immutable,	/* CPU: -/- | GPU: R/- */
				Usage_Renderbuffer,	/* CPU: -/- | GPU: R/W */

				Usage_Count
			};

			enum Format
			{
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

			struct LockResult
			{
				void* data;
				unsigned int rowPitch;
				unsigned int slicePitch;
			};

			~Texture();

			virtual void upload(unsigned int index, unsigned int mip, const TextureRegion& region, const void* data, unsigned int size) = 0;

			virtual bool download(unsigned int index, unsigned int mip, void* data, unsigned int size) = 0;

			virtual bool supportsLocking() const = 0;
			virtual LockResult lock(unsigned int index, unsigned int mip, const TextureRegion& region) = 0;
			virtual void unlock(unsigned int index, unsigned int mip) = 0;

			virtual shared_ptr<Renderbuffer> getRenderbuffer(unsigned int index, unsigned int mip) = 0;

			virtual void commitChanges() = 0;
			virtual void generateMipmaps() = 0;

			Type getType() const { return type; }
			Format getFormat() const { return format; }

			unsigned int getWidth() const { return width; }
			unsigned int getHeight() const { return height; }
			unsigned int getDepth() const { return depth; }

			unsigned int getMipLevels() const { return mipLevels; }

			Usage getUsage() const { return usage; }

			static bool isFormatCompressed(Format format);
			static bool isFormatDepth(Format format);
			static unsigned int getFormatBits(Format format);
			static unsigned int getImageSize(Format format, unsigned int width, unsigned int height);

			static unsigned int getTextureSize(Type type, Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels);

			static unsigned int getMipSide(unsigned int value, unsigned int mip);
			static unsigned int getMaxMipCount(unsigned int width, unsigned int height, unsigned int depth);

		protected:
			Texture(Device* device, Type type, Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels, Usage usage);

			Type type;
			Format format;
			unsigned int width;
			unsigned int height;
			unsigned int depth;
			unsigned int mipLevels;
			Usage usage;
		};

	}
}
