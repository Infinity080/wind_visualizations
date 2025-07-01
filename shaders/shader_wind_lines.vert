#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 3) in mat4 instanceMatrix;
layout (location = 7) in float windSpeed;

uniform mat4 viewProjectionMatrix;
uniform float time;
uniform float maxWindSpeed;

out float vWindSpeed;
out float vAnimationProgress;
out float vLocalPosition;

void main()
{
    vWindSpeed = windSpeed;
    
    // Offset dla ka¿dej linii na podstawie pozycji
    float animationOffset = fract((instanceMatrix[3][0] + instanceMatrix[3][1] + instanceMatrix[3][2]) * 43758.5453);
    
    // Prêdkoœæ animacji zale¿na od prêdkoœci wiatru
    float speedNormalized = clamp(windSpeed / maxWindSpeed, 0.1, 1.0);
    float animationSpeed = 0.2 + speedNormalized * 2;
    
    // D³ugoœæ cyklu animacji uzale¿niona od prêdkoœci wiatru
    float cycleLength = 2.9 + speedNormalized * 2.0; // Od 3 do 5 sekund
    
    float animationTime = mod(time * animationSpeed + animationOffset * 6.28, cycleLength);
    
    // Normalizacja postêpu do zakresu [0, 3] niezale¿nie od rzeczywistej d³ugoœci cyklu
    vAnimationProgress = (animationTime / cycleLength) * 3.0;
    
    // Pozycja lokalna wzd³u¿ linii (0 = pocz¹tek, 1 = koniec)
    vLocalPosition = aPos.x + 0.5; // Przekszta³cenie z [-0.5, 0.5] na [0, 1]
    
    gl_Position = viewProjectionMatrix * instanceMatrix * vec4(aPos, 1.0);
}