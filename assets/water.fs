#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float time;
uniform vec2 camera_target;
uniform vec2 camera_offset;
uniform float camera_zoom;

out vec4 finalColor;

void main() {
    vec4 texel = texture(texture0, fragTexCoord);

    // Reconstruct world position from screen-space fragment coords
    vec2 world_pos = (gl_FragCoord.xy - camera_offset) / camera_zoom + camera_target;

    // Brightness wave: two overlapping sin waves for organic feel
    float wave1 = sin(world_pos.x * 0.08 + world_pos.y * 0.04 + time * 1.2) * 0.5 + 0.5;
    float wave2 = sin(world_pos.x * 0.05 - world_pos.y * 0.07 + time * 0.8) * 0.5 + 0.5;
    float brightness = mix(0.88, 1.08, wave1 * 0.6 + wave2 * 0.4);

    // Subtle blue-green tint shift
    float tint_wave = sin(world_pos.x * 0.03 + time * 0.5) * 0.5 + 0.5;
    vec3 water_tint = mix(vec3(0.85, 0.92, 1.0), vec3(0.88, 1.0, 0.95), tint_wave);

    vec3 result = texel.rgb * brightness * water_tint;
    finalColor = vec4(result, texel.a) * colDiffuse * fragColor;
}
