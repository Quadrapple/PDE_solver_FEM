#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;

out vec2 vertPos;
out vec3 TexCoords;

void main() {
    gl_Position = vec4(aPos.x, aPos.y, 1.0, 1.0);
    TexCoords = aColor;
}
