#pragma once
#include "aabb.h"
#include "material.h"
#include <memory>
#include <globjects/globjects.h>

using BufferRef = std::unique_ptr<globjects::Buffer>;
using VertexArrayRef = std::unique_ptr<globjects::VertexArray>;

struct Mesh
{
	AABB box;
	MaterialRef material;
	uint32_t index_count = 0;

	BufferRef vbo;
	BufferRef ebo;
	VertexArrayRef vao;
};

using MeshRef = std::shared_ptr<Mesh>;