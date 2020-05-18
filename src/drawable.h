#pragma once

#include "matrix_stack.h"

class IDrawable
{
public:
    virtual void draw(const MatrixStack& sceneTransforms) = 0;
    virtual ~IDrawable() {}
};