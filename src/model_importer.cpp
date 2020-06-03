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
#include <cassert>

#include <glbinding/gl/gl.h>
#include <globjects/globjects.h>
#include <globjects/VertexArray.h>
#include <globjects/VertexAttributeBinding.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "utils.h"

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
		aiProcess_GenBoundingBoxes |
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
	functional_unique_ptr<unsigned char> image_ptr {
	    SOIL_load_image_from_memory(buffer_ptr, buffer_size, &w, &h, &c, SOIL_LOAD_RGBA),
	    [](unsigned char* ptr) {SOIL_free_image_data(ptr); }
	};

	if (!image_ptr)
	{
		throw std::runtime_error(SOIL_last_result());
	}
	
	texture->bindActive(0);
	texture->image2D(0, gl::GL_RGBA, glm::ivec2(w, h), 0, gl::GL_RGBA, gl::GL_UNSIGNED_BYTE, image_ptr.get());

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

MaterialRef parse_material(aiMaterial* assimp_material, const TextureList& textures, const aiScene* scene)
{
	MaterialRef material = std::make_shared<Material>();

	const static std::tuple<aiTextureType, unsigned, std::string_view> knownTextureFlavours[] = 
	{
	    {aiTextureType_DIFFUSE, 0, "u_texAlbedo"sv},
		{AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE, "u_texAlbedo"sv},
		{aiTextureType_NORMALS, 0, "u_texNormalMap"sv},
		{aiTextureType_HEIGHT, 0, "u_texNormalMap"sv},
		{AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, "u_texAoRoughnessMetallic"sv}
	};

	for (auto [type, index, name] : knownTextureFlavours)
	{
		if (material->textures.find(name) != material->textures.end())
			continue;

		if (aiString path; assimp_material->GetTexture(type, index, &path) == aiReturn_SUCCESS && path.length > 0)
		{
			if (auto index = get_texture_index(path.data, scene); index >= 0)
			{
				material->textures.emplace(name, textures[index]);
			}
		}
	}

	return material;
}

using MaterialList = std::vector<MaterialRef>;

MaterialList parse_materials(aiMaterial** assimp_materials, uint32_t material_count, const TextureList& textures,
	const aiScene* scene)
{
	MaterialList materials;

	for (uint32_t i = 0; i < material_count; ++i)
	{
		materials.emplace_back(parse_material(assimp_materials[i], textures, scene));
	}

	return materials;
}

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 bitangent;
	glm::vec2 uv;
};

glm::vec3 to_glm(const aiVector3D& v)
{
	return { v.x, v.y, v.z };
}

glm::mat4 to_glm(const aiMatrix4x4& m)
{
	return glm::transpose(glm::make_mat4(&m.a1));
}

BufferRef parse_vbo(aiMesh* assimp_mesh)
{
	BufferRef vbo = globjects::Buffer::create();

	std::vector<Vertex> data;
	data.resize(assimp_mesh->mNumVertices);

	for (uint32_t i = 0; i < assimp_mesh->mNumVertices; ++i)
	{
		auto& vertex = data[i];
		vertex.position = to_glm(assimp_mesh->mVertices[i]);
		vertex.normal = to_glm(assimp_mesh->mNormals[i]);
		vertex.tangent = to_glm(assimp_mesh->mTangents[i]);
		vertex.bitangent = to_glm(assimp_mesh->mBitangents[i]);
		vertex.uv = to_glm(assimp_mesh->mTextureCoords[0][i]);
	}

	vbo->bind(gl::GL_VERTEX_ARRAY);
	vbo->setData(data, gl::GL_STATIC_DRAW);

	return vbo;
}

BufferRef parse_ebo(aiMesh* assimp_mesh, uint32_t& index_count)
{
	BufferRef ebo = globjects::Buffer::create();

	std::vector<uint32_t> data;
	data.reserve(static_cast<size_t>(assimp_mesh->mNumFaces * 3));

	for (uint32_t i = 0; i < assimp_mesh->mNumFaces; ++i)
	{
		auto& face = assimp_mesh->mFaces[i];

		for (uint32_t j = 0; j < face.mNumIndices; ++j)
		{
			data.emplace_back(face.mIndices[j]);
		}
	}

	ebo->bind(gl::GL_ELEMENT_ARRAY_BUFFER);
	ebo->setData(data, gl::GL_STATIC_DRAW);

	index_count = static_cast<uint32_t>(data.size());

	return ebo;
}

MeshRef parse_mesh(aiMesh* assimp_mesh, const MaterialList& materials)
{
	MeshRef mesh = std::make_shared<Mesh>();

	assert(assimp_mesh->HasPositions());
	assert(assimp_mesh->HasTextureCoords(0));
	assert(assimp_mesh->HasNormals());
	assert(assimp_mesh->HasTangentsAndBitangents());

	mesh->vbo = parse_vbo(assimp_mesh);
	mesh->ebo = parse_ebo(assimp_mesh, mesh->index_count);

	auto& assimp_aabb = assimp_mesh->mAABB;

	mesh->box.min = to_glm(assimp_aabb.mMin);
	mesh->box.max = to_glm(assimp_aabb.mMax);

	mesh->vao = globjects::VertexArray::create();
	mesh->vao->bindElementBuffer(mesh->ebo.get());

	auto position_binding = mesh->vao->binding(0);
	position_binding->setAttribute(0);
	position_binding->setBuffer(mesh->vbo.get(), offsetof(Vertex, position), sizeof(Vertex));
	position_binding->setFormat(3, gl::GL_FLOAT, false);
	mesh->vao->enable(0);

	auto normal_binding = mesh->vao->binding(1);
	normal_binding->setAttribute(1);
	normal_binding->setBuffer(mesh->vbo.get(), offsetof(Vertex, normal), sizeof(Vertex));
	normal_binding->setFormat(3, gl::GL_FLOAT, false);
	mesh->vao->enable(1);

	auto tangent_binding = mesh->vao->binding(2);
	tangent_binding->setAttribute(2);
	tangent_binding->setBuffer(mesh->vbo.get(), offsetof(Vertex, tangent), sizeof(Vertex));
	tangent_binding->setFormat(3, gl::GL_FLOAT, false);
	mesh->vao->enable(2);

	auto bitangent_binding = mesh->vao->binding(3);
	bitangent_binding->setAttribute(3);
	bitangent_binding->setBuffer(mesh->vbo.get(), offsetof(Vertex, bitangent), sizeof(Vertex));
	bitangent_binding->setFormat(3, gl::GL_FLOAT, false);
	mesh->vao->enable(3);

	auto uv_binding = mesh->vao->binding(4);
	uv_binding->setAttribute(4);
	uv_binding->setBuffer(mesh->vbo.get(), offsetof(Vertex, uv), sizeof(Vertex));
	uv_binding->setFormat(2, gl::GL_FLOAT, false);
	mesh->vao->enable(4);

	mesh->vao->unbind();

	assert(assimp_mesh->mMaterialIndex < materials.size());
	mesh->material = materials[assimp_mesh->mMaterialIndex];

	return mesh;
}

using MeshList = std::vector<MeshRef>;

MeshList parse_meshes(aiMesh** assimp_meshes, uint32_t mesh_count, const MaterialList& materials)
{
	MeshList meshes;

	for (uint32_t i = 0; i < mesh_count; ++i)
	{
		meshes.emplace_back(parse_mesh(assimp_meshes[i], materials));
	}

	return meshes;
}

NodeRef parse_nodes(aiNode* assimp_node, const MeshList& meshes)
{
	NodeRef node = std::make_shared<Node>();

	node->transform = to_glm(assimp_node->mTransformation);

	for (uint32_t i = 0; i < assimp_node->mNumMeshes; ++i)
	{
		auto mesh_index = assimp_node->mMeshes[i];
		assert(mesh_index < meshes.size());

		node->meshes.emplace_back(meshes[mesh_index]);
	}

	for (uint32_t i = 0; i < assimp_node->mNumChildren; ++i)
	{
		node->childs.emplace_back(parse_nodes(assimp_node->mChildren[i], meshes));
	}

	return node;
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
		auto meshes = parse_meshes(scene->mMeshes, scene->mNumMeshes, materials);

		return parse_nodes(scene->mRootNode, meshes);
	}
	catch (const std::exception& e)
	{
		spdlog::error(e.what());
	}

	return nullptr;
}
