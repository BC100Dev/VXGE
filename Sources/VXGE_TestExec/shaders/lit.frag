#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D albedoTex;

// Matches VXRenderer's GPULight struct exactly: vec3 direction + float intensity,
// vec3 color + padding, laid out as two implicit vec4s so std430 can't misalign the stride.
struct LightData {
    vec3 direction;
    float intensity;
    vec3 color;
    float _pad;
};

layout(set = 2, binding = 0) readonly buffer LightBuffer {
    uint count;
    // std140/std430 pads the next member to a 16-byte boundary regardless of this field's
    // own size, so the array below always starts at byte offset 16 -- matching GPULight's
    // layout in VXRenderer.cpp exactly (count occupies one float-sized slot of padding there).
    LightData lights[];
} lightBuffer;

// Directional Blinn-Phong. Camera position isn't passed to this shader yet, so the
// specular term uses the light direction as a view-independent approximation rather than
// a real half-vector -- good enough for now, and the first thing to fix once VXCamera's
// world position is threaded through.
const float AMBIENT = 0.1;
const float SHININESS = 16.0;

void main() {
    vec4 albedo = texture(albedoTex, fragUV) * fragColor;
    vec3 N = normalize(fragNormal);

    vec3 lighting = vec3(AMBIENT);

    for (uint i = 0; i < lightBuffer.count; i++) {
        vec3 L = normalize(-lightBuffer.lights[i].direction);
        float diff = max(dot(N, L), 0.0);

        vec3 H = normalize(L + N);
        float spec = pow(max(dot(N, H), 0.0), SHININESS);

        lighting += lightBuffer.lights[i].color * lightBuffer.lights[i].intensity * (diff + spec);
    }

    outColor = vec4(albedo.rgb * lighting, albedo.a);
}
