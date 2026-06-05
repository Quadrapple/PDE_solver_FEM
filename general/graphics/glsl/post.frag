#version 330 core

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform vec3 thresholds;

uniform vec3 lowColor;
uniform vec3 midColor;
uniform vec3 highColor;

out vec4 FragColor;

vec3 interpolateColors(float val, vec3 first, vec3 second, vec3 third, vec3 thresholds_) {
    val = clamp(val, thresholds_.x, thresholds_.z);

    float tx = thresholds_.x;
    float ty = thresholds_.y;
    float tz = thresholds_.z;

    if (val < ty) {
        float w = (ty > tx) ? clamp(1.0 - (val - tx) / (ty - tx), 0.0, 1.0) : 1.0;
        return w * first + (1.0 - w) * second;
    } else {
        float w = (tz > ty) ? clamp(1.0 - (val - ty) / (tz - ty), 0.0, 1.0) : 1.0;
        return w * second + (1.0 - w) * third;
    }
}

void main() {
    vec2 txt = texture(screenTexture, TexCoords).xy;
    float val = txt.x - txt.y;
    vec3 color = interpolateColors(val, lowColor, midColor, highColor, thresholds);
    FragColor = vec4(color, 1.0);
    
}
