#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float time;

out vec4 finalColor;

void main() {
    vec2 uv = fragTexCoord;

    // UV displacement (same technique as water)
    float wave_speed = 0.6;
    float wave_freq = 12.0;
    float wave_height = 0.008;

    float phase_x = (uv.y + time * wave_speed) * wave_freq;
    float phase_y = (uv.x + time * wave_speed) * wave_freq;

    uv.x += sin(phase_x) * cos(phase_x * 0.5) * wave_height;
    uv.y += sin(phase_y) * cos(phase_y * 0.5) * wave_height * 0.5;

    uv = clamp(uv, 0.0, 1.0);

    vec4 texel = texture(texture0, uv);

    // Blue tint + semi-transparent
    texel.b = min(texel.b + 0.15, 1.0);
    texel.a *= 0.5;

    finalColor = texel * colDiffuse * fragColor;
}
