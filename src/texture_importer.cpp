#include "texture_importer.h"

#include <glbinding/gl/enum.h>

TextureRef TextureImporter::load(const std::string& filename)
{
    gli::texture image = gli::load(filename);
	if (image.empty())
		return {};

	gli::gl glTranslator{ gli::gl::PROFILE_GL33 };
	const auto imageFormat = glTranslator.translate(image.format(), image.swizzles());
	auto imageTarget = static_cast<gl::GLenum>(glTranslator.translate(image.target()));

	auto texture = globjects::Texture::createDefault(imageTarget);
	texture->setParameter(gl::GL_TEXTURE_BASE_LEVEL, 0);
	texture->setParameter(gl::GL_TEXTURE_MAX_LEVEL, static_cast<gl::GLint>(image.levels() - 1));
	texture->setParameter(gl::GL_TEXTURE_SWIZZLE_R, imageFormat.Swizzles[0]);
	texture->setParameter(gl::GL_TEXTURE_SWIZZLE_G, imageFormat.Swizzles[1]);
	texture->setParameter(gl::GL_TEXTURE_SWIZZLE_B, imageFormat.Swizzles[2]);
	texture->setParameter(gl::GL_TEXTURE_SWIZZLE_A, imageFormat.Swizzles[3]);

	texture->bind();

	glm::tvec3<gl::GLsizei> extent{ image.extent() };
    const auto faceTotal = static_cast<gl::GLsizei>(image.layers() * image.faces());

	switch (image.target())
	{
	case gli::TARGET_1D:
		texture->storage1D(static_cast<gl::GLint>(image.levels()), static_cast<gl::GLenum>(imageFormat.Internal), extent.x);
		break;
	case gli::TARGET_1D_ARRAY:
	case gli::TARGET_2D:
	case gli::TARGET_CUBE:
        texture->storage2D(
			static_cast<gl::GLint>(image.levels()), static_cast<gl::GLenum>(imageFormat.Internal),
			extent.x, image.target() == gli::TARGET_1D_ARRAY ? faceTotal : extent.y);
		break;
	case gli::TARGET_2D_ARRAY:
	case gli::TARGET_3D:
	case gli::TARGET_CUBE_ARRAY:
        texture->storage3D(
			static_cast<gl::GLint>(image.levels()), static_cast<gl::GLenum>(imageFormat.Internal),
			extent.x, extent.y,
			image.target() == gli::TARGET_3D ? extent.z : faceTotal);
		break;
	default:
		assert(0);
		break;
	}

	for (std::size_t layer = 0; layer < image.layers(); ++layer)
		for (std::size_t face = 0; face < image.faces(); ++face)
			for (std::size_t level = 0; level < image.levels(); ++level)
			{
                auto const layerGL = static_cast<gl::GLsizei>(layer);
				const glm::tvec3<gl::GLsizei> extent = { image.extent(level) };
				imageTarget = gli::is_target_cube(image.target())
					? static_cast<gl::GLenum>(gl::GLenum::GL_TEXTURE_CUBE_MAP_POSITIVE_X + face)
					: imageTarget;

				switch (image.target())
				{
				case gli::TARGET_1D:
					if (gli::is_compressed(image.format()))
                        gl::glCompressedTexSubImage1D(
							imageTarget, static_cast<gl::GLint>(level), 0, extent.x,
							static_cast<gl::GLenum>(imageFormat.Internal), static_cast<gl::GLsizei>(image.size(level)),
							image.data(layer, face, level));
					else
						texture->subImage1D( static_cast<gl::GLint>(level), 0, extent.x,
							static_cast<gl::GLenum>(imageFormat.External), static_cast<gl::GLenum>(imageFormat.Type),
							image.data(layer, face, level));
					break;
				case gli::TARGET_1D_ARRAY:
				case gli::TARGET_2D:
				case gli::TARGET_CUBE:
					if (gli::is_compressed(image.format()))
				         gl::glCompressedTexSubImage2D(
							 imageTarget, static_cast<gl::GLint>(level),
							 0, 0,
							 extent.x,
							 image.target() == gli::TARGET_1D_ARRAY ? layerGL : extent.y,
							 static_cast<gl::GLenum>(imageFormat.Internal), static_cast<gl::GLsizei>(image.size(level)),
							 image.data(layer, face, level));
					else
						gl::glTexSubImage2D(
							imageTarget,
					        static_cast<gl::GLint>(level),
							0, 0,
							extent.x,
							image.target() == gli::TARGET_1D_ARRAY ? layerGL : extent.y,
							static_cast<gl::GLenum>(imageFormat.External), static_cast<gl::GLenum>(imageFormat.Type),
							image.data(layer, face, level));
					break;
				case gli::TARGET_2D_ARRAY:
				case gli::TARGET_3D:
				case gli::TARGET_CUBE_ARRAY:
					if (gli::is_compressed(image.format()))
                        gl::glCompressedTexSubImage3D(
							imageTarget, static_cast<gl::GLint>(level),
							0, 0, 0,
							extent.x, extent.y,
							image.target() == gli::TARGET_3D ? extent.z : layerGL,
							static_cast<gl::GLenum>(imageFormat.Internal), static_cast<gl::GLsizei>(image.size(level)),
							image.data(layer, face, level));
					else
                        texture->subImage3D(
					        static_cast<gl::GLint>(level),
							0, 0, 0,
							extent.x, extent.y,
							image.target() == gli::TARGET_3D ? extent.z : layerGL,
							static_cast<gl::GLenum>(imageFormat.External), static_cast<gl::GLenum>(imageFormat.Type),
							image.data(layer, face, level));
					break;
				default: assert(0); break;
				}
			}


	return texture;
}
