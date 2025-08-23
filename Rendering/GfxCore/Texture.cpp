#include "GfxCore/Texture.h"

#include "rbx/Profiler.h"

#include <array>

FASTFLAGVARIABLE(GraphicsTextureCommitChanges, false)

namespace RBX {
	namespace Graphics {

		struct FormatDescription {
			uint32_t bpp;
			bool compressed;
			bool depth;
		};

		static const std::array<FormatDescription, Texture::Format_Count> gTextureFormats = { {
			{  8u, false, false }, /* R8         */
			{ 16u, false, false }, /* RG8        */
			{ 32u, false, false }, /* RGBA8      */
			{ 16u, false, false }, /* BGR5A1     */
			{ 32u, false, false }, /* R11G11B10f */
			{ 32u, false, false }, /* RG16       */
			{ 32u, false, false }, /* RG16f      */
			{ 64u, false, false }, /* RGBA16f    */

			{  4u,  true, false }, /* BC1        */
			{  4u,  true, false }, /* BC1_sRGB   */
			{  8u,  true, false }, /* BC2        */
			{  8u,  true, false }, /* BC2_sRGB   */
			{  8u,  true, false }, /* BC3        */
			{  8u,  true, false }, /* BC3_sRGB   */
			{  4u,  true, false }, /* BC4        */
			{  8u,  true, false }, /* BC5        */
			{  8u,  true, false }, /* BC6f       */
			{  8u,  true, false }, /* BC7        */
			{  8u,  true, false }, /* BC7_sRGB   */

			{ 16u, false,  true }, /* D16        */
			{ 32u, false,  true }, /* D24S8      */
			{ 32u, false,  true }, /* D32f       */
			{ 64u, false,  true }, /* D32fS8X24  */
		} };

		TextureRegion::TextureRegion()
			: x(0u)
			, y(0u)
			, z(0u)
			, width(0u)
			, height(0u)
			, depth(0u)
		{
		}

		TextureRegion::TextureRegion(uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height, uint32_t depth)
			: x(x)
			, y(y)
			, z(z)
			, width(width)
			, height(height)
			, depth(depth)
		{
		}

		TextureRegion::TextureRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
			: x(x)
			, y(y)
			, z(0u)
			, width(width)
			, height(height)
			, depth(1u)
		{
		}

		Texture::Texture()
			: Resource()
			, type(Type_Count)
			, format(Format_Count)
			, width(0u)
			, height(0u)
			, depth(0u)
			, mipLevels(0u)
			, usage(Usage_Count)
		{
		}

		Texture::Texture(Device* device, Type type, Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, Usage usage)
			: Resource(device)
			, type(type)
			, format(format)
			, width(width)
			, height(height)
			, depth(depth)
			, mipLevels(mipLevels)
			, usage(usage)
		{
			RBXASSERT(width > 0u && height > 0u && depth > 0u);
			RBXASSERT(mipLevels > 0u && mipLevels <= getMaxMipCount(width, height, depth));
			RBXASSERT(type == Type_3D || depth == 1u);

			if (usage != Usage_Colorbuffer) {
				RBXPROFILER_COUNTER_ADD("memory/gpu/texture", getTextureSize(type, format, width, height, depth, mipLevels));
			}
		}

		Texture::~Texture() {
			if (usage != Usage_Colorbuffer) {
				RBXPROFILER_COUNTER_SUB("memory/gpu/texture", getTextureSize(type, format, width, height, depth, mipLevels));
			}
		}

		bool Texture::isFormatCompressed(Format format) {
			return gTextureFormats[format].compressed;
		}

		bool Texture::isFormatDepth(Format format) {
			return gTextureFormats[format].depth;
		}

		uint32_t Texture::getFormatBits(Format format) {
			return gTextureFormats[format].bpp;
		}

		uint32_t Texture::getImageSize(Format format, uint32_t width, uint32_t height) {
			const FormatDescription& desc = gTextureFormats[format];

			switch (format) {
			case Format_BC1:
			case Format_BC2:
			case Format_BC3:
				return std::max(1u, ((width + 3u) / 4u)) * std::max(1u, ((height + 3u) / 4u)) * (desc.bpp * 16u / 8u);

			default:
				return width * height * (desc.bpp / 8u);
			}
		}

		uint32_t Texture::getTextureSize(Type type, Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels) {
			uint32_t result = 0u;

			for (uint32_t mip = 0u; mip < mipLevels; ++mip)
				result += Texture::getImageSize(format, Texture::getMipSide(width, mip), Texture::getMipSide(height, mip)) * Texture::getMipSide(depth, mip);

			return result * (type == Texture::Type_Cube ? 6u : 1u);
		}

		uint32_t Texture::getMipSide(uint32_t value, uint32_t mip) {
			uint32_t result = value >> mip;

			return result ? result : 1u;
		}

		uint32_t Texture::getMaxMipCount(uint32_t width, uint32_t height, uint32_t depth) {
			uint32_t result = 0u;

			while (width || height || depth) {
				result++;

				width /= 2u;
				height /= 2u;
				depth /= 2u;
			}

			return result;
		}

	}
}
