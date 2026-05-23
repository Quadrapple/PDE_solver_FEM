#include "texture.h"
#include "state.h"
#include <iostream>

Texture::Texture() : id(0), mapType{DIFFUSE}, type{GL_TEXTURE_2D}, path(NULL), size(0) {}

Texture::Texture(GLenum type, GLenum internalFormat, glm::uvec2 size, unsigned int samples, bool fixedSamplePosition) : id(0), type(type), size(size) {
    glGenTextures(1, &id);

    texImage2DMult(internalFormat, samples, fixedSamplePosition);

    glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

Texture::Texture(GLenum type, GLenum internalFormat, glm::uvec2 size, GLenum origFormat, const unsigned char *data, std::string path) : id(0), type(type), size(size),path(path) {
    glGenTextures(1, &id);

    texImage2D(internalFormat, origFormat, data);
    glGenerateMipmap(type);

    glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void Texture::bind(unsigned int unit) const {
    State::getContext().bindTexture(type, unit, id);
}

void Texture::texParameter(GLenum param, GLenum value) {
    bind(0);
    glTexParameteri(type, param, value);
}

void Texture::texImage2D(GLenum internalFormat, GLenum origFormat, const unsigned char *data) {
    bind(0);
    glTexImage2D(type, 0, internalFormat, size.x, size.y, 0, origFormat, GL_UNSIGNED_BYTE, data);
}

void Texture::texImage2DMult(GLenum internalFormat, unsigned int samples, bool fixedSampleLocations) {
    bind(0);
    glTexImage2DMultisample(type, samples, internalFormat, size.x, size.y, fixedSampleLocations);
}

unsigned int Texture::getId() const {
    return id;
}

std::string Texture::getPath() const {
    return path;
}

GLenum Texture::getType() const {
    return type;
}

glm::uvec2 Texture::getSize() const {
    return size;
}

Texture::~Texture() {
    glDeleteTextures(1, &id);
}
