#pragma once
#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wall"
#pragma clang diagnostic ignored "-Wdeprecated-volatile"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#ifdef WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
#endif
#include <GL/glew.h>
/* Ask for an OpenGL Core Context */

#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

// //

#include <cstdint>
#include <fstream>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <initializer_list>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Vendor stuff
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "../vendor/stb_image.h"
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "../vendor/stb_truetype.h"
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../vendor/stb_image_write.h"
#define FMT_HEADER_ONLY
#include "../vendor/fmt/format.h"
#include "../vendor/fmt/ostream.h"
// this is needed for wstring printing
#include "../vendor/fmt/xchar.h"
#define GLT_IMPLEMENTATION
#include "../vendor/glText.h"
#include "../vendor/portable-file-dialogs.h"
//
#include "../vendor/Quadtree.h"

#ifdef __APPLE__
#pragma clang diagnostic pop
#else
#pragma enable_warn
#endif

#ifdef WIN32
#pragma GCC diagnostic pop
#endif

