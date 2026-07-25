#version 330 core

in vec3 vNormal;
in vec3 vFragPos;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 color;

void main()
{
    // ambient
    vec3 ambient = 0.2 * color; 

    // diffuse
    vec3 N = normalize(vNormal);
    vec3 L = normalize(lightPos - vFragPos);
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * color;

    // specular
    vec3 V = normalize(viewPos - vFragPos);
    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), 32.0);
    vec3 specular = vec3(0.6) * spec;

    vec3 finalColor = ambient + diffuse + specular;
    vec3 finalColor = color * 0.7 + result * 0.3;
    FragColor = vec4(finalColor, 1.0);
}
