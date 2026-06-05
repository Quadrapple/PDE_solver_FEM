#version 330 core

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float diffstep;
uniform vec3 thresholds;

uniform vec3 lowColor;
uniform vec3 midColor;
uniform vec3 highColor;

out vec4 FragColor;

float f(vec2 tc) {
    return -100*tc.y;
}

float laplacian(float ds, vec2 tc) {
    float res = 0.0;
    float dx = ds/2;
    float dy = ds/2;

    res -= 4*texture(screenTexture, tc).x;
    res += texture(screenTexture, vec2(tc.x, tc.y + dy)).x;
    res += texture(screenTexture, vec2(tc.x, tc.y - dy)).x;
    res += texture(screenTexture, vec2(tc.x - dx, tc.y)).x;
    res += texture(screenTexture, vec2(tc.x + dx, tc.y)).x;

    res /= ds;
    res /= ds;

    if( ((tc.x + dx) > 1.0f) || ((tc.y + dy )> 1.0f) || ((tc.x - dx) < 0.0f) || ((tc.y - dy) < 0.0f) ) {
        return 0.0;
    }

    return res - f(2*TexCoords - 1.0);
}

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
    float val = laplacian(diffstep, TexCoords);
    vec3 color = interpolateColors(val, lowColor, midColor, highColor, thresholds);
    FragColor = vec4(color, 1.0);
    
}

