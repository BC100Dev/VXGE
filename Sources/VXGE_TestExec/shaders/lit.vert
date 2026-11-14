#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragNormal;

layout(set = 0, binding = 0) uniform UBO {
    mat4 mvp;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

void main() {
    gl_Position = ubo.mvp * push.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragUV = inUV;

    // Valid for rotation/uniform-scale/translation model matrices (everything VXGE builds
    // today). A true inverse-transpose is needed only once non-uniform scale shows up.
    fragNormal = normalize(mat3(push.model) * inNormal);
}
