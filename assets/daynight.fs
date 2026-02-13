#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float time_of_day; // 0.0 – 24.0
uniform vec2 screen_size;  // window dimensions in pixels
uniform vec2 light_pos;    // player center in screen pixels (0,0 = top-left)
uniform float light_radius; // outer radius in screen pixels (0 = no light)

out vec4 finalColor;

// Attempt a smoothstep without using built-in (avoids potential GLSL issues)
vec3 get_tint(float t) {
    // Key colors
    vec3 night = vec3(0.2, 0.2, 0.45);
    vec3 dawn  = vec3(0.9, 0.55, 0.4);
    vec3 day   = vec3(1.0, 0.95, 0.9);
    vec3 dusk  = vec3(0.9, 0.5, 0.3);

    // Deep night: 0–4
    // Dawn transition: 4–7
    // Day: 7–17
    // Dusk transition: 17–20
    // Night: 20–24

    float dawn_factor = smoothstep(4.0, 5.5, t) * (1.0 - smoothstep(5.5, 7.0, t));
    float day_factor  = smoothstep(5.5, 7.0, t) * (1.0 - smoothstep(17.0, 18.5, t));
    float dusk_factor = smoothstep(17.0, 18.5, t) * (1.0 - smoothstep(18.5, 20.0, t));
    float night_factor = 1.0 - dawn_factor - day_factor - dusk_factor;

    return night * night_factor + dawn * dawn_factor + day * day_factor + dusk * dusk_factor;
}

void main() {
    vec4 texel = texture(texture0, fragTexCoord);
    vec3 scene_tint = get_tint(time_of_day);

    // Torch light
    vec2 frag_px = fragTexCoord * screen_size;
    float dist = distance(frag_px, light_pos);
    float inner = light_radius * 0.3;
    float light_factor = (light_radius > 0.0) ? (1.0 - smoothstep(inner, light_radius, dist)) : 0.0;

    vec3 torch_tint = vec3(1.0, 0.9, 0.7);
    vec3 final_tint = mix(scene_tint, torch_tint, light_factor);

    finalColor = vec4(texel.rgb * final_tint, texel.a) * colDiffuse * fragColor;
}
