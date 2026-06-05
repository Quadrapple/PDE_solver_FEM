#version 330 core

in vec2 vertPos;
in float colorval;

void main() {
    if (distance(gl_PointCoord , vec2(0.5, 0.5)) > 0.5) {
        discard;
    }
    FragColor = vec4(1.0);
}
