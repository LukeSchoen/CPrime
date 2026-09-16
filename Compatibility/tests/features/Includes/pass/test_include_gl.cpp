#include <GL/gl.h>

int main()
{
  GLuint id = 0;
  GLenum target = GL_TEXTURE_2D;
  return id == 0 && target == GL_TEXTURE_2D ? 0 : 1;
}
