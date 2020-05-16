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

TextureRef parse_texture(aiTexture* assimp_texture)
{
	TextureRef texture = globjects::Texture::create();

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

using TextureList = std::vector<TextureRef>;

TextureList parse_textures(aiTexture** assimp_textures, uint32_t texture_count)
{
	TextureList textures;

	for (uint32_t i = 0; i < texture_count; ++i)
	{
		textures.emplace_back(parse_texture(assimp_textures[i]));
	}

	return textures;
}

int get_texture_index(const char* name, const aiScene* scene)
{
	if ('*' == *name) 
	{
		int index = std::atoi(name + 1);

		if (0 > index || scene->mNumTextures <= static_cast<unsigned>(index))
		{
			return -1;
		}
		return index;
	}

	// lookup using filename
	const char* shortFilename = scene->GetShortFilename(name);
	for (unsigned int i = 0; i < scene->mNumTextures; i++) 
	{
		const char* shortTextureFilename = scene->GetShortFilename(scene->mTextures[i]->mFilename.C_Str());
		if (strcmp(shortTextureFilename, shortFilename) == 0) 
		{
			return static_cast<int>(i);
		}
	}

	spdlog::error("Can't parse texture index");
	return -1;
}

MaterialRef parse_material(aiMaterial* assimp_material, TextureList textures, const aiScene* scene)
{
	MaterialRef material = std::make_shared<Material>();

	//DIFFUSE
	{
		aiString path;
		if (assimp_material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
		{
			if (path.length > 0)
			{
				auto index = get_texture_index(path.data, scene);

				if (index >= 0)
				{
					material->albedo = textures[index];
				}
			}
		}

		if (!material->albedo)
		{
			if (assimp_material->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE,
				&path) == AI_SUCCESS)
			{
				if (path.length > 0)
				{
					auto index = get_texture_index(path.data, scene);

					if (index >= 0)
					{
						material->albedo = textures[index];
					}
				}
			}
		}
	}

	//NORMALMAP
	{
		aiString path;
		if (assimp_material->GetTexture(aiTextureType_NORMALS, 0, &path) == AI_SUCCESS)
		{
			if (path.length > 0)
			{
				auto index = get_texture_index(path.data, scene);

				if (index >= 0)
				{
					material->normalMap = textures[index];
				}
			}
		}

		if (!material->normalMap)
		{
			if (assimp_material->GetTexture(aiTextureType_HEIGHT, 0, &path) == AI_SUCCESS)
			{
				if (path.length > 0)
				{
					auto index = get_texture_index(path.data, scene);

					if (index >= 0)
					{
						material->normalMap = textures[index];
					}
				}
			}
		}
	}

	//AO_METALLIC_ROUGHNESS
	{
		aiString path;
		if (assimp_material->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE,
			&path) == AI_SUCCESS)
		{
			if (path.length > 0)
			{
				auto index = get_texture_index(path.data, scene);

				if (index >= 0)
				{
					material->aoRoughnessMetallic = textures[index];
				}
			}
		}
	}

	return material;
}

using MaterialList = std::vector<MaterialRef>;

MaterialList parse_materials(aiMaterial** assimp_materials, uint32_t material_count, TextureList textures,
	const aiScene* scene)
{
	MaterialList materials;

	for (uint32_t i = 0; i < material_count; ++i)
	{
		materials.emplace_back(parse_material(assimp_materials[i], textures, scene));
	}

	return materials;
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
		auto materials = parse_materials(scene->mMaterials, scene->mNumMaterials, textures, scene);
	}
	catch (const std::exception& e)
	{
		spdlog::error(e.what());
	}

	return nullptr;
}
