#pragma once
#include "mesh.h"
#include <vector>
#include <glm/glm.hpp>

using NodeRef = std::shared_ptr<struct Node>;

struct Node
{
	glm::mat4 transform;
	std::vector<MeshRef> meshes;
	std::vector<NodeRef> childs;
};