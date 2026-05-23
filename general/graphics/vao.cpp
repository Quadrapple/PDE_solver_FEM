#include "vao.h"
#include "buffer.h"
#include "state.h"
#include <memory>

VertexArray::VertexArray() : attribCount(0) {
    glGenVertexArrays(1, &id);
}

VertexArray::VertexArray(std::unique_ptr<Buffer> vertices) {
    glGenVertexArrays(1, &id);
    bind();
    setVertices(std::move(vertices));
    setDefaultAttribs();
}

void VertexArray::bind() const {
    State::getContext().bindVertexArray(id);
}

VertexArray& VertexArray::operator<<(const VertexAttrib &attrib) {
    bind();
    glEnableVertexAttribArray(attribCount);
    glVertexAttribPointer(attribCount, attrib.count, attrib.type, GL_FALSE, attrib.stride, attrib.ptr);
    attribCount++;
    return *this;
}

void VertexArray::addAttrib(const VertexAttrib &attrib) {
    bind();
    glEnableVertexAttribArray(attribCount);
    glVertexAttribPointer(attribCount, attrib.count, attrib.type, GL_FALSE, attrib.stride, attrib.ptr);
    attribCount++;
}

void VertexArray::setVertices(std::unique_ptr<Buffer> buf) {
    bind();
    buf->bind();
    vertices.swap(buf); //Assign the unique_ptr
}

void VertexArray::setIndices(std::unique_ptr<Buffer> buf) {
    bind();
    buf->bind();
    indices.swap(buf); //Assign the unique_ptr
}

void VertexArray::setDefaultAttribs() {
    bind();
    vertices->bind();
    attribCount = 3;
    //Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)offsetof(Vertex, position));

    //Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)offsetof(Vertex, normal)); 

    //TexCoord
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)offsetof(Vertex, texCoords));
}

void VertexArray::enableAttrib(unsigned int index) {
    bind();
    glEnableVertexAttribArray(index);
}

void VertexArray::disableAttrib(unsigned int index) {
    bind();
    glEnableVertexAttribArray(index);
}

unsigned int VertexArray::getId() const {
    return id;
}
