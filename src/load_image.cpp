#include "load_image.hpp"

#include <stdexcept>

/**
 * This allows us to handle stb_image assertions via exceptions on the python side.
 */
void checkStbImageAssertion(bool expr, const char* exprStr) {

    if (!expr) { 
        throw std::runtime_error(exprStr);
    }
}

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT(_EXPR) checkStbImageAssertion(_EXPR, #_EXPR)

#include "stb_image.h"

unsigned char* loadImage(const char* filename, int* x, int* y, int* n, int wantedChannels) {
    
    return stbi_load(filename, x, y, n, wantedChannels);
}
