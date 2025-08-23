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
				Type_2DMS,
				Type_2D_Array,
				Type_2DMS_Array,
				Type_3D,
				Type_Cube,
				Type_Cube_Array,

				Type_Count
			};

			enum Format {
				Format_R8,			/* R8  --- --- --- |  8 bits |       */
				Format_RG8,			/* R8  G8  --- --- | 16 bits |       */
				Format_RGBA8,		/* R8  G8  B8  A8  | 32 bits |       */
				Format_BGR5A1,		/* B5  G5  R5  A1  | 16 bits |       */
				Format_R11G11B10f,	/* R11 G11 B10 --- | 32 bits | float */
				Format_RG16,		/* R16 G16 --- --- | 32 bits |       */
				Format_RG16f,		/* R16 G16 --- --- | 32 bits | float */
				Format_RGBA16f,		/* R16 G16 B16 A16 | 64 bits | float */

				Format_BC1,			/* R5  G6  B5  A1  |  4 bits         */
				Format_BC1_sRGB,	/* R5  G6  B5  A1  |  4 bits | sRGB  */
				Format_BC2,			/* R5  G6  B5  A4  |  8 bits         */
				Format_BC2_sRGB,	/* R5  G6  B5  A4  |  8 bits | sRGB  */
				Format_BC3,			/* R5  G6  B5  A8  |  8 bits         */
				Format_BC3_sRGB,	/* R5  G6  B5  A8  |  8 bits | sRGB  */
				Format_BC4,			/* R8  --- --- --- |  4 bits         */
				Format_BC5,			/* R8  G8  --- --- |  8 bits         */
				Format_BC6f,		/* R-- G-- B-- --- |  8 bits | float */
				Format_BC7,			/* R-- G-- B-- A-- |  8 bits         */
				Format_BC7_sRGB,	/* R-- G-- B-- A-- |  8 bits | sRGB  */

				Format_D16,			/* D16 --- ---     | 16 bits |       */
				Format_D24S8,		/* D24 S8  ---     | 32 bits |       */
				Format_D32f,		/* D32 --- ---     | 32 bits | float */
				Format_D32fS8X24,	/* D32 S8  X24     | 64 bits | float */

				Format_Count
			};

			/* CPU: READ/WRITE | GPU: READ/WRITE/SHADER */
			enum Usage {
				Usage_Static,		/* CPU: -/- | GPU: R/W/S */
				Usage_Dynamic,		/* CPU: -/W | GPU: R/-/S */
				Usage_Immutable,	/* CPU: -/- | GPU: R/-/S | Note: Must be initialized with data. */
				Usage_Colorbuffer,	/* CPU: -/- | GPU: R/W/- */
				Usage_Depthbuffer,	/* CPU: -/- | GPU: R/W/- */
				Usage_Colortexture, /* CPU: -/- | GPU: R/W/S */
				Usage_Depthtexture, /* CPU: -/- | GPU: R/W/S */

				Usage_Count,
			};

			struct LockResult {
				void* data;
				uint32_t rowPitch;
				uint32_t slicePitch;
			};

			Texture();
			~Texture();

			virtual void upload(uint32_t index, uint32_t mip, const TextureRegion& region, const void* data, size_t size) = 0;

			virtual bool download(uint32_t index, uint32_t mip, void* data, size_t size) = 0;

			virtual bool supportsLocking() const = 0;
			virtual LockResult lock(uint32_t index, uint32_t mip, const TextureRegion& region) = 0;
			virtual void unlock(uint32_t index, uint32_t mip) = 0;

			virtual std::shared_ptr<Renderbuffer> getRenderbuffer(uint32_t index, uint32_t mip) = 0;

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

			struct TextureStage {
				TextureStage()
					: stage(0u)
					, texture(nullptr)
				{
				}

				TextureStage(uint32_t stage, std::shared_ptr<Texture> texture)
					: stage(stage)
					, texture(texture)
				{
				}

				uint32_t stage;
				std::shared_ptr<Texture> texture;
			};

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
