#ifndef OBJECT_H
#define OBJECT_H

#include "model.h"

class Object {
    public:
        Model *model;

        glm::vec3 position;
        glm::vec3 scaleFactor;

        Object(Model *model, glm::vec3 position);
        Object(Model *model, glm::vec3 position, glm::mat4 rotation);
        Object(Model *model, glm::vec3 position, glm::mat4 rotation, glm::vec3 scaling);

        void rotate(glm::vec3 axis, float angle);
        void move(glm::vec3 vector);
        void scale(glm::vec3 scaling);

        glm::mat4 modelMat() const;
        void draw(Shader &shader);

    private:
        glm::mat4 rotationMatrix;
        glm::mat4 modelMatrix;
};

#endif
