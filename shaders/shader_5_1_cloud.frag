#version 430 core

float AMBIENT = 0.3;

in vec3 worldPos;
in vec2 texCoord;

out vec4 outColor;
in vec3 vecNormal;

uniform vec3 lightPos;
uniform vec3 cameraPos;
uniform sampler2D cloudColor;
uniform float shininess = 30.0;

void main() {
    vec3 N = normalize(vecNormal);
    vec3 L = normalize(lightPos - worldPos);
    vec3 V = normalize(cameraPos - worldPos);
    vec3 H = normalize(L + V); // Halfway vector dla specular

    // === Diffuse Lighting ===
    // Wplyw swiatla slonecznego na podstawie normalnej wierzcholka
    float sunInfluence = clamp(dot(N, L), 0.0, 1.0);

    // === Specular Lighting ===
    // Podstawowy specular na podstawie normalnej wierzcholka
    float spec = pow(max(dot(N, H), 0.0), shininess);
    vec3 specularColor = vec3(1.0) * spec * sunInfluence * 0.3; // Zmniejszona intensywnosc

    // === Cloud Shading & Alpha ===
    vec4 cloudSample = texture(cloudColor, texCoord); // Probkujemy raz dla koloru i alpha
    vec3 cloudTex = cloudSample.rgb;
    // Oswietlenie chmury (ambient + diffuse)
    // Uzyj stalej AMBIENT zdefiniowanej wczesniej w shaderze
    vec3 cloudLit = cloudTex * (AMBIENT + sunInfluence);

    // Polacz oswietlenie diffuse/ambient i specular
    vec3 finalColor = cloudLit + specularColor;

    // === Alpha control ===
    // Uzyj kanalu alpha bezposrednio z tekstury
    float finalAlpha = cloudSample.a;

    // Zwieksz jasnosc i ustaw alpha
    outColor = vec4(finalColor * 2.5, finalAlpha * 0.8);
    }