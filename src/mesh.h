#pragma once
#include "aabb.h"
#include <memory>
#include <globjects/globjects.h>

struct Mesh
{
	std::unique_ptr<globjects::Buffer> vbo;
	std::unique_ptr<globjects::Buffer> ebo;
	std::unique_ptr<globjects::VertexArray> vao;
	uint32_t index_count;
	AABB box;
};

using MeshRef = std::shared_ptr<Mesh>;