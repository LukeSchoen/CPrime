#include <GL/gl.h>

static GLfloat CapturedRed = 0.0f;
static GLfloat CapturedGreen = 0.0f;
static GLfloat CapturedBlue = 0.0f;
static GLfloat CapturedAlpha = 0.0f;
static const GLubyte Version[] = "test-version";

int main()
{
    glClearColor(0.25f, 0.5f, 0.75f, 1.0f);
    const GLubyte* version = glGetString(GL_VENDOR);

    return CapturedRed == 0.25f
        && CapturedGreen == 0.5f
        && CapturedBlue == 0.75f
        && CapturedAlpha == 1.0f
        && version == Version ? 0 : 1;
}

extern "C" void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    CapturedRed = red;
    CapturedGreen = green;
    CapturedBlue = blue;
    CapturedAlpha = alpha;
}

extern "C" const GLubyte* glGetString(GLenum)
{
    return Version;
}
