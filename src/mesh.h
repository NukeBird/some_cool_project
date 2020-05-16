#pragma once
#include "aabb.h"
#include <memory>
#include <globjects/globjects.h>

using BufferRef = std::unique_ptr<globjects::Buffer>;
using VertexArrayRef = std::unique_ptr<globjects::VertexArray>;

struct Mesh
{
	BufferRef vbo;
	BufferRef ebo;
	VertexArrayRef vao;
	uint32_t index_count;
	AABB box;
};

using MeshRef = std::shared_ptr<Mesh>;