#version 330 core

in vec3 TexCoords;
in float colorVal;

out float FragColor;

void main() {
    FragColor = colorVal;
}
