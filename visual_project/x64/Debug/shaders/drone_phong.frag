#version 330 core

in vec3 vNormal;
in vec3 vFragPos;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 color;
uniform float uSparkleIntensity; // 이제는 "발광 세기"로 사용

void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(lightPos - vFragPos);
    vec3 V = normalize(viewPos - vFragPos);
    vec3 R = reflect(-L, N);

    // -----------------------------
    // 기본 조명 (물리적 반사)
    // -----------------------------
    vec3 ambient  = color * 0.25;
    float diff    = max(dot(N, L), 0.0);
    vec3 diffuse  = color * diff * 0.9;
    vec3 specular = vec3(0.2) * pow(max(dot(V, R), 0.0), 24.0);

    vec3 lighting = ambient + diffuse + specular;

    // -----------------------------
    // ⭐ Emissive (자체 발광)
    // -----------------------------
    // 색 기반 발광 → 흰색 안 됨
    vec3 emissive = color * (0.4 + uSparkleIntensity * 0.6);

    // -----------------------------
    // 최종 색상
    // -----------------------------
    vec3 finalColor = lighting + emissive;

    FragColor = vec4(finalColor, 1.0);
}
