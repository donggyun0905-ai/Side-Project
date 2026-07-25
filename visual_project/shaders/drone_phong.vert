#version 330 core

layout(location = 0) in vec3 aPos;      // 위치
layout(location = 1) in vec3 aNormal;   // 노말

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vNormal;
out vec3 vFragPos;

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos = worldPos.xyz;

    // 노말 변환 (정석)
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;

    gl_Position = uProj * uView * worldPos;
}
