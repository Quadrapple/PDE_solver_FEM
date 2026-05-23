#include "shader.h"

#include "state.h"
#include "shader_loader.h"

Shader::Shader(std::string vertexSource, std::string fragmentSource) {
    id = ShaderLoader::createShader(vertexSource, fragmentSource);
}

void Shader::use() { 
    State::getContext().useProgram(id);
}

//Uniform setters
void Shader::setBool(const std::string &name, bool value) const {
    glUniform1i(glGetUniformLocation(id, name.c_str()), (int)value);
}
//--------------------------------------------------------------------
void Shader::setInt(const std::string &name, int value) const {
    glUniform1i(glGetUniformLocation(id, name.c_str()), value);
}
//--------------------------------------------------------------------
void Shader::setFloat(const std::string &name, float value) const {
    glUniform1f(glGetUniformLocation(id, name.c_str()), value);
}
//--------------------------------------------------------------------
void Shader::setMat4(const std::string &name, glm::mat4 matrix) const {
    glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
}
//--------------------------------------------------------------------
void Shader::setVec2(const std::string &name, glm::vec2 vector) const {
    glUniform2fv(glGetUniformLocation(id, name.c_str()), 1, &vector[0]);
}

void Shader::setVec2(const std::string &name, float x, float y) const {
    glUniform2f(glGetUniformLocation(id, name.c_str()), x, y);
}
//--------------------------------------------------------------------
void Shader::setVec3(const std::string &name, glm::vec3 vector) const {
    glUniform3fv(glGetUniformLocation(id, name.c_str()), 1, &vector[0]);
}

void Shader::setVec3(const std::string &name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(id, name.c_str()), x, y, z);
}
//--------------------------------------------------------------------
void Shader::setVec4(const std::string &name, glm::vec4 vector) const {
    glUniform4fv(glGetUniformLocation(id, name.c_str()), 1, &vector[0]);
}

void Shader::setVec4(const std::string &name, float x, float y, float z, float w) const {
    glUniform4f(glGetUniformLocation(id, name.c_str()), x, y, z, w);
}
//--------------------------------------------------------------------
void Shader::bindUniformBlock(const std::string &name, unsigned int bindingPoint) const {
    unsigned int index = glGetUniformBlockIndex(id, name.c_str());
    glUniformBlockBinding(id, index, bindingPoint);
}
