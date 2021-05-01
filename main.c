// standard includes
#include <GL/glew.h>
#include <GL/freeglut.h>

// OpenGL global variables
static unsigned int window_width = 1280, window_height = 720;

int main(int argc, char* argv[])
{
    glutInit(&argc, argv);

    // set OpenGL version
    glutInitContextVersion(4, 6);
    // remove deprecated functions to make sure the program is compatible with future versions of OpenGL
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);

    // create window
    glutInitWindowSize(window_width, window_height);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("Infinite Procedural Terrain Generator");

    glewInit();

    glutMainLoop();

    return 0;
}
