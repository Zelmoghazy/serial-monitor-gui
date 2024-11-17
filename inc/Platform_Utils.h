#ifndef PLATFORM_UTILS_H
#define PLATFORM_UTILS_H

#include <string>
#include <windows.h>
#include "commdlg.h"
#include "glfw3.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include "glfw3native.h"

namespace Windows_file_dialog
{
    std::string OpenFile(const char* filter,GLFWwindow *window);

    std::string SaveFile(const char* filter,GLFWwindow *window);
}

#endif