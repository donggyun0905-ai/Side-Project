#version 330 core

in vec3 vNormal;
in vec3 vFragPos;
out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 color;
uniform float uSparkleIntensity;

float hash(vec3 p)
{
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

void main()
{
    vec3 N = normalize(vNormal);
    
    // 노말이 0이면 diffuse 안 나옴 → 색 죽는 문제 해결
    if (length(N) < 0.1)
        N = vec3(0.0, 1.0, 0.0);

    vec3 L = normalize(lightPos - vFragPos);
    float diff = max(dot(N, L), 0.0);

    // diffuse 위주로 색을 확실하게 표현
    vec3 diffuse = color * (0.3 + diff * 0.8);

    vec3 ambient = color * 0.2;

    vec3 V = normalize(viewPos - vFragPos);
    vec3 R = reflect(-L, N);

    float spec = pow(max(dot(V, R), 0.0), 16.0);
    vec3 specular = vec3(1.0) * spec * 0.15;

    vec3 baseColor = ambient + diffuse + specular;

    // sparkle
    float n = hash(vFragPos * 5.0);
    float threshold = 1.0 - (uSparkleIntensity * 0.85);
    float sparkle = (n > threshold ? 1.0 : 0.0) * uSparkleIntensity;

    baseColor += vec3(1.0) * sparkle;

    FragColor = vec4(baseColor, 1.0);
}
