#pragma once
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include <vector>
#include "material.h"
#include "node.h"

class ModelImporter
{
public:
	static NodeRef load(const std::string& filename);
};