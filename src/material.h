#pragma once
#include <memory>
#include <globjects/globjects.h>

using TextureRef = std::shared_ptr<globjects::Texture>;

struct Material
{
	TextureRef albedo;
	TextureRef normalMap;
	TextureRef aoRoughnessMetallic;
};

using MaterialRef = std::shared_ptr<Material>;