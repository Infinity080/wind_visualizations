#version 430 core

uniform vec3 cameraPos;
uniform vec3 lightPos;
uniform vec3 atmosphereColor;
uniform float intensity;

in vec3 vecNormal;
in vec3 worldNormal;
in vec3 worldPos;

out vec4 outColor;

void main() {

    outColor = vec4(1.0);

    // Uzywamy interpolowanej normalnej wierzcholka
    vec3 N = normalize(vecNormal);
    vec3 V = normalize(cameraPos - worldPos); // Wektor do kamery
    vec3 L = normalize(lightPos - worldPos); // Wektor do swiatla

    // Oblicz wspolczynnik rim (poswiaty na krawedzi)
    float dotVN = dot(V, N);
    float rim = 1.0 - dotVN; // 0 w centrum, ~1 na krawedzi
    float glow = pow(clamp(rim, 0.0, 1.0), 7.0); // Kontroluje szerokosc poswiaty

    // Oblicz wplyw swiatla slonecznego (przyciemnij nocna strone)
    float sunInfluence = clamp(dot(N, L), 0.0, 1.0);
    
    // Moduluj poswiate przez wplyw slonca
    glow *= sunInfluence;

    // Kolor koncowy to kolor atmosfery * poswiata * intensywnosc
    vec3 finalColor = atmosphereColor * glow * intensity;
    
    // Ustaw kolor RGB i alpha (alpha moze byc uzyte do kontroli blendingu)
    // Dla GL_ONE, GL_ONE alpha jest ignorowane, ale ustawienie glow moze byc uzyteczne
    outColor = vec4(finalColor, glow);
}
