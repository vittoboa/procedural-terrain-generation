#include <GL/glew.h>
#include <GL/freeglut.h>
#include <stdio.h>
#include <string.h>

// function to read text file
static char* readTextFile(char* aTextFile)
{
    FILE* filePointer = fopen(aTextFile, "rb");
    char* content;
    long numVal;

    fseek(filePointer, 0L, SEEK_END);
    numVal = ftell(filePointer);
    fseek(filePointer, 0L, SEEK_SET);
    content = (char*) malloc((numVal+1) * sizeof(char));
    fread(content, 1, numVal, filePointer);
    content[numVal] = '\0';
    fclose(filePointer);

    return content;
}

// function to initialize shaders
int setShader(char* shaderType, char* shaderFile)
{
    int shaderId;
    char* shader = readTextFile(shaderFile);

    // set shaderId based on the shader type
    if (strcmp(shaderType, "tessEvaluation") == 0) shaderId = glCreateShader(GL_TESS_EVALUATION_SHADER);
    if (strcmp(shaderType, "tessControl"   ) == 0) shaderId = glCreateShader(GL_TESS_CONTROL_SHADER);
    if (strcmp(shaderType, "geometry"      ) == 0) shaderId = glCreateShader(GL_GEOMETRY_SHADER);
    if (strcmp(shaderType, "fragment"      ) == 0) shaderId = glCreateShader(GL_FRAGMENT_SHADER);
    if (strcmp(shaderType, "vertex"        ) == 0) shaderId = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(shaderId, 1, (const char**) &shader, NULL);
    glCompileShader(shaderId);

    return shaderId;
}
