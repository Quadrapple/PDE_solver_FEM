#include <stdexcept>
#include <fstream>
#include <sstream>
#include <format>
#include "shader_loader.h"

unsigned int ShaderLoader::createShader(std::string vertexSource, std::string fragmentSource) {
    std::string vCode = loadFile(vertexSource.c_str());
    std::string fCode = loadFile(fragmentSource.c_str());

    unsigned int vShader = compileVertexShader(vCode.c_str());
    unsigned int fShader = compileFragmentShader(fCode.c_str());

    return createShaderProgram(vShader, fShader);
}

std::string ShaderLoader::loadFile(const char *filename) {
    std::ifstream sourceFile;
    std::string source;

    sourceFile.exceptions(std::ifstream::failbit | std::fstream::badbit);

    try {
        sourceFile.open(filename);

        std::stringstream sourceStream;
        sourceStream << sourceFile.rdbuf();
        source = sourceStream.str();

        sourceFile.close();

    } catch (const std::ifstream::failure& e) {
        printf("ERROR! SHADER::FAILED_FILE_READ %s\n", filename);
    }

    return source;
}

unsigned int ShaderLoader::compileVertexShader(const char* source) {
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &source, NULL);
    glCompileShader(vertexShader);

    checkCompile(vertexShader, "VERTEX");

    return vertexShader;
}

unsigned int ShaderLoader::compileFragmentShader(const char* source) {
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &source, NULL);
    glCompileShader(fragmentShader);

    checkCompile(fragmentShader, "FRAGMENT");

    return fragmentShader;
}

void ShaderLoader::checkCompile(unsigned int id, std::string type) {
    int success;
    char infoLog[512];

    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(id, 512, NULL, infoLog);
        throw std::runtime_error(std::format("Failed to compile {} shader: {}", type, infoLog));
    }
}

unsigned int ShaderLoader::createShaderProgram(unsigned int vertexShader, unsigned int fragmentShader) {
    unsigned int id = glCreateProgram();
    glAttachShader(id, vertexShader);
    glAttachShader(id, fragmentShader);
    glLinkProgram(id);

    checkLink(id);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return id;
}

void ShaderLoader::checkLink(unsigned int id) {
    int success;
    char infoLog[512];
    glGetProgramiv(id, GL_LINK_STATUS, &success); 
    if(!success) {
        glGetProgramInfoLog(id, 512, NULL, infoLog);
        throw std::runtime_error(std::format("Failed to link shader: {}", infoLog));
    }
}

