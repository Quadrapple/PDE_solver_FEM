#ifndef VAO_H
#define VAO_H

#include "buffer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

struct VertexAttrib {
    unsigned int count;
    unsigned int stride;
    GLenum type;
    const void *ptr;
};

class VertexArray {
    public:
        VertexArray();

        //Sets default attributes
        VertexArray(std::unique_ptr<Buffer> vertices); 

        void bind() const;

        void setVertices(std::unique_ptr<Buffer> buf);
        void setIndices(std::unique_ptr<Buffer> buf);

        void enableAttrib(unsigned int index);
        void disableAttrib(unsigned int index);

        unsigned int getId() const;
        void setDefaultAttribs();
        void addAttrib(const VertexAttrib &attrib);

        unsigned int indexCount;
        VertexArray& operator<<(const VertexAttrib &attrib);
    private:
        unsigned int attribCount;
        unsigned int id;

        std::unique_ptr<Buffer> vertices;
        std::unique_ptr<Buffer> indices;

};

#endif
