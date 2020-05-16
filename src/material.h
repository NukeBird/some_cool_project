#pragma once
#include <memory>
#include <globjects/globjects.h>

struct Material
{
	std::shared_ptr<globjects::Texture> albedo;
	std::shared_ptr<globjects::Texture> normalMap;
	std::shared_ptr<globjects::Texture> aoRoughnessMetallic;
};

using MaterialRef = std::shared_ptr<Material>;