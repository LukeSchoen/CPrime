#include <GL/gl.h>

int main()
{
    return GL_REPEAT == 0x2901
        && GL_NEAREST_MIPMAP_NEAREST == 0x2700
        && GL_LINEAR_MIPMAP_LINEAR == 0x2703
        && GL_DEPTH_COMPONENT == 0x1902
        && GL_TEXTURE_1D == 0x0DE0
        && GL_NEAREST == 0x2600
        && GL_RGBA == 0x1908
        && GL_UNSIGNED_BYTE == 0x1401
        && GL_TEXTURE_2D == 0x0DE1
        && GL_LESS == 0x0201
        && GL_SRC_ALPHA == 0x0302
        && GL_ONE_MINUS_SRC_ALPHA == 0x0303
        && GL_VENDOR == 0x1F00
        && GL_RENDERER == 0x1F01 ? 0 : 1;
}
