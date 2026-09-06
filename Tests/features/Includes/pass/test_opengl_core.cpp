// EXPECT_COMPILE_ONLY: 1
#include <GL/gl.h>
static_assert(GL_VIEWPORT == 0x0BA2, "viewport enum");
static_assert(GL_DEPTH_FUNC == 0x0B74, "depth enum");
static_assert(GL_SCISSOR_BOX == 0x0C10, "scissor enum");
void query() {
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  glDepthFunc(GL_LEQUAL);
  glScissor(viewport[0], viewport[1], viewport[2], viewport[3]);
}
