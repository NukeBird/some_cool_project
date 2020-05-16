#include "model_importer.h"
#include <assimp/Importer.hpp>
#include <assimp/cimport.h>
#include <assimp/config.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>
#include <spdlog/spdlog.h>
#include <SOIL2/SOIL2.h>
#include <stdexcept>

uint32_t get_importer_flags()
{
	return aiProcess_GenSmoothNormals | //can't be specified with aiProcess_GenNormals
		aiProcess_FindInstances |
		aiProcess_FindDegenerates |
		aiProcess_FindInvalidData |
		aiProcess_CalcTangentSpace |
		aiProcess_ValidateDataStructure |
		aiProcess_OptimizeMeshes |
		aiProcess_OptimizeGraph |
		aiProcess_JoinIdenticalVertices |
		aiProcess_ImproveCacheLocality |
		aiProcess_LimitBoneWeights |
		aiProcess_RemoveRedundantMaterials |
		aiProcess_SplitLargeMeshes |
		aiProcess_Triangulate |
		aiProcess_GenUVCoords |
		aiProcess_GenBoundingBoxes | //TODO
		aiProcess_SplitByBoneCount |
		aiProcess_SortByPType |
		aiProcess_FlipUVs;
}

std::shared_ptr<globjects::Texture> parse_texture(aiTexture* assimp_texture)
{
	std::shared_ptr<globjects::Texture> texture = globjects::Texture::create();

	texture->setParameter(gl::GL_TEXTURE_MIN_FILTER, gl::GL_LINEAR_MIPMAP_LINEAR);
	texture->setParameter(gl::GL_TEXTURE_MAG_FILTER, gl::GL_LINEAR);
	texture->setParameter(gl::GL_TEXTURE_WRAP_S, gl::GL_CLAMP_TO_EDGE);
	texture->setParameter(gl::GL_TEXTURE_WRAP_T, gl::GL_CLAMP_TO_EDGE);
	texture->setParameter(gl::GL_TEXTURE_WRAP_R, gl::GL_CLAMP_TO_EDGE);

	auto buffer_ptr = reinterpret_cast<unsigned char*>(assimp_texture->pcData);
	auto buffer_size = assimp_texture->mWidth * (assimp_texture->mHeight > 0 ? assimp_texture->mHeight : 1);

	int w, h, c;
	auto image_ptr = SOIL_load_image_from_memory(buffer_ptr, buffer_size, &w, &h, &c, SOIL_LOAD_RGBA);

	if (!image_ptr)
	{
		SOIL_free_image_data(image_ptr);
		throw std::runtime_error(SOIL_last_result());
	}

	texture->image2D(0, gl::GL_RGBA, glm::ivec2(w, h), 0, gl::GL_RGBA, gl::GL_UNSIGNED_BYTE, image_ptr);

	SOIL_free_image_data(image_ptr);
	texture->generateMipmap();

	spdlog::info("Texture {0}x{1} loaded", w, h);

	return texture;
}

std::vector<std::shared_ptr<globjects::Texture>> parse_textures(aiTexture** assimp_textures, uint32_t texture_count)
{
	std::vector<std::shared_ptr<globjects::Texture>> textures;

	for (uint32_t i = 0; i < texture_count; ++i)
	{
		textures.emplace_back(parse_texture(assimp_textures[i]));
	}

	return textures;
}

NodeRef ModelImporter::load(const std::string& filename)
{
	Assimp::Importer importer;

	importer.SetPropertyInteger(AI_CONFIG_IMPORT_TER_MAKE_UVS, 1);
	importer.SetPropertyInteger(AI_CONFIG_PP_SBBC_MAX_BONES, 120);
	importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80.0f);
	importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);

	auto scene = importer.ReadFile(filename, get_importer_flags());

	if (!scene)
	{
		spdlog::error(importer.GetErrorString());
		return nullptr;
	}

	try
	{
		auto textures = parse_textures(scene->mTextures, scene->mNumTextures);
	}
	catch (const std::exception& e)
	{
		spdlog::error(e.what());
	}

	return nullptr;
}
