#include "object.h"

Object::Object(Model *model, glm::vec3 position) {
    this->position = position;
    this->model = model;
    this->scaleFactor = glm::vec3(1.0);
    rotationMatrix = glm::mat4(1.0);
}

Object::Object(Model *model, glm::vec3 position, glm::mat4 rotation) {
    this->position = position;
    this->model = model;
    this->scaleFactor = glm::vec3(1.0);
    rotationMatrix = glm::mat4(1.0);
}

Object::Object(Model *model, glm::vec3 position, glm::mat4 rotation, glm::vec3 scaling) {
    this->position = position;
    this->model = model;
    this->scaleFactor = scaling;    
    rotationMatrix = rotation;
}

void Object::rotate(glm::vec3 axis, float angle) {
    rotationMatrix = glm::rotate(rotationMatrix, angle, axis);
}

void Object::move(glm::vec3 vector) {
    //printf("X: %.2f, Y: %.2f, Z: %.2f\n", position.x, position.y, position.z);
    position += vector;
}

void Object::scale(glm::vec3 scaling) {
    scaleFactor *= scaling;
}

glm::mat4 Object::modelMat() const{
    return glm::translate(glm::mat4(1.0), position) * glm::scale(glm::mat4(1.0), scaleFactor) * rotationMatrix;
}

void Object::draw(Shader &shader) {
    model->draw(shader);
}

