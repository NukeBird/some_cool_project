#pragma once

#include <memory>
#include <globjects/globjects.h>
#include <gli/gli.hpp>

using TextureRef = std::shared_ptr<globjects::Texture>;

class TextureImporter
{
public:
    static TextureRef load(const std::string& filename);
};
