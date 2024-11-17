#pragma once

#include "glew.h"


class Image
{
public:
    GLuint image_texture;
    int width;
    int height;

    Image(const char* filename);
    bool GetImageFromPath(const char* filename);
};