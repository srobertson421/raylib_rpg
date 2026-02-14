#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float time_of_day; // 0.0 – 24.0
uniform vec2 screen_size;  // window dimensions in pixels
uniform vec2 light_pos;    // player center in screen pixels (0,0 = top-left)
uniform float light_radius; // outer radius in screen pixels (0 = no light)

// Cloud shadow uniforms
uniform float cloud_intensity;  // 0-1, overall shadow darkness (modulated by time of day)
uniform vec2 cloud_offset;      // World-space offset for animation (drifts over time)
uniform float cloud_scale;      // Controls cloud size (lower = bigger clouds)
uniform vec2 camera_target;     // Camera position in world
uniform vec2 camera_offset;     // Screen center offset
uniform float camera_zoom;      // Camera zoom level

out vec4 finalColor;

// Simple hash function for noise
vec2 hash(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

// Simplex-like noise
float noise(vec2 p) {
    const float K1 = 0.366025404; // (sqrt(3)-1)/2
    const float K2 = 0.211324865; // (3-sqrt(3))/6

    vec2 i = floor(p + (p.x + p.y) * K1);
    vec2 a = p - i + (i.x + i.y) * K2;
    vec2 o = (a.x > a.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    vec2 b = a - o + K2;
    vec2 c = a - 1.0 + 2.0 * K2;

    vec3 h = max(0.5 - vec3(dot(a, a), dot(b, b), dot(c, c)), 0.0);
    vec3 n = h * h * h * h * vec3(dot(a, hash(i)), dot(b, hash(i + o)), dot(c, hash(i + 1.0)));

    return dot(n, vec3(70.0));
}

// Layered noise for cloud-like shapes (FBM - Fractal Brownian Motion)
float cloud_noise(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    // 4 octaves for cloud detail
    for (int i = 0; i < 4; i++) {
        value += amplitude * noise(p * frequency);
        amplitude *= 0.5;
        frequency *= 2.0;
    }

    return value;
}

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

    // Cloud shadows
    vec3 result = texel.rgb * final_tint;
    if (cloud_intensity > 0.0 && camera_zoom > 0.0) {
        // Reconstruct world position (same technique as water.fs)
        vec2 screen_pos = vec2(gl_FragCoord.x, screen_size.y - gl_FragCoord.y);
        vec2 world_pos = (screen_pos - camera_offset) / camera_zoom + camera_target;

        // Generate cloud shadow pattern
        vec2 cloud_pos = (world_pos + cloud_offset) * cloud_scale;
        float cloud_value = cloud_noise(cloud_pos);

        // Remap noise to shadow (higher threshold = fewer, more distinct clouds)
        float shadow = smoothstep(0.35, 0.55, cloud_value);  // Creates 2-10 large clouds
        shadow *= cloud_intensity;

        // Apply shadow darkening (max 40% darkening)
        result *= (1.0 - shadow * 0.4);
    }

    finalColor = vec4(result, texel.a) * colDiffuse * fragColor;
}
