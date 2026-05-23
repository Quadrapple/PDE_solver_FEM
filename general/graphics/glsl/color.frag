#version 330 core

in vec3 TexCoords;

out vec4 FragColor;

void main() {
    vec4 result = vec4(TexCoords, 1.0);
    FragColor = result;
}
