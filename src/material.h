#pragma once
#include <memory>
#include <globjects/globjects.h>

using TextureRef = std::shared_ptr<globjects::Texture>;

struct Material
{
    std::map<std::string, TextureRef, std::less<>> textures; 
};

using MaterialRef = std::shared_ptr<Material>;