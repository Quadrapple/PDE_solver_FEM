#ifndef SHADER_LOADER_H
#define SHADER_LOADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>

class ShaderLoader {
    public:
        static unsigned int createShader(std::string vertexSource, std::string fragmentSource);
    private:
        static unsigned int createShaderProgram(unsigned int vertexShader, unsigned int fragmentShader);
        static void checkLink(unsigned int id);

        static unsigned int compileVertexShader(const char *source);
        static unsigned int compileFragmentShader(const char *source);
        static void checkCompile(unsigned int id, std::string type);

        static std::string loadFile(const char *filename);
};

#endif
