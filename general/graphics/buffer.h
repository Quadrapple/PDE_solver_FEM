#ifndef BUFFER_H
#define BUFFER_H

#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

class Buffer {
    public:
        Buffer();
        Buffer(GLenum type, unsigned int size, const void* data);

        ~Buffer();

        template <typename T>
        Buffer(GLenum type, const std::vector<T> &data);

        void bind() const;
        void bindAs(GLenum target);
        void bindBufferBase(unsigned int index);
        void bindBufferBaseAs(GLenum target, unsigned int index);

        template <typename T>
        void bufferData(std::vector<T> const &data);

        template <typename T>
        void bufferSubData(unsigned int offset, std::vector<T> const &data) const;

        void bufferData(unsigned int size, const void* data);
        void bufferSubData(unsigned int offset, unsigned int size, const void* data) const;

        unsigned int getId() const;
        unsigned int getSize() const;

    private:
        unsigned int id;
        unsigned int size;
        GLenum type;
};

template<typename T>
Buffer::Buffer(GLenum type, std::vector<T> const &data) {
    glGenBuffers(1, &id);
    this->type = type;
    bufferData(data);
}

template<typename T>
void Buffer::bufferData(std::vector<T> const &data) {
    size = sizeof(T) * data.size();
    bufferData(size, data.data());
}

template<typename T>
void Buffer::bufferSubData(unsigned int offset, std::vector<T> const &data) const {
    bufferSubData(offset,  sizeof(T) * data.size(), data.data());
}

#endif
