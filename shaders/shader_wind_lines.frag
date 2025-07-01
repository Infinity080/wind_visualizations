#version 330 core

in float vWindSpeed;
in float vAnimationProgress;
in float vLocalPosition;

uniform vec3 baseColor;
uniform float maxWindSpeed;
uniform float time;

out vec4 FragColor;

void main()
{   
    // Parametry animacji linii wiatru
    float baseAlpha = 1.0;
    
    // Normalizacja prêdkoœci wiatru do zakresu [0.1, 1.0]
    float speedNormalized = clamp(vWindSpeed / maxWindSpeed, 0.1, 1.0);
    
    // D³ugoœæ lini zale¿na od prêdkoœci wiatru
    float lineLength = 1.0 + speedNormalized * 1.5; // Zakres od 1.1 do 2.5
    
    float wavePosition = vAnimationProgress - lineLength;
    float visibilityAlpha = 0.0;
    
    if (vLocalPosition >= wavePosition && vLocalPosition <= wavePosition + lineLength) {
        float relativePos = (vLocalPosition - wavePosition) / lineLength;
        
        // Oblicz pozycjê w œladzie
        float trailPosition = relativePos;
        
        float trailFactor;
        
        if (trailPosition <= 0.3) {
            // Pocz¹tek linii - pe³na intensywnoœæ z lekkim fade-in
            trailFactor = smoothstep(0.0, 0.1, trailPosition);
        } else if (trailPosition <= 0.7) {
            // Œrodek linii - stopniowy spadek
            float midProgress = (trailPosition - 0.3) / 0.4;
            trailFactor = 1.0 - pow(midProgress, 0.8) * 0.3; // Delikatny spadek
        } else {
            // Koniec linii - zanikanie do 0
            float endProgress = (trailPosition - 0.7) / 0.3;
            
            // Oblicz wartoœæ na pocz¹tku tej sekcji
            float startValue = 1.0 - pow(1.0, 0.8) * 0.3; // = 0.7
            
            // Szybszy wiatr = szybsze zanikanie
            float fadeExponent = 1.5 + speedNormalized * 1.0; // Zakres od 1.6 do 2.5
            trailFactor = startValue * pow(1.0 - endProgress, fadeExponent);
        }
        
        visibilityAlpha = trailFactor;
        
        // Ograniczenie do przedzia³u [0, 1]
        visibilityAlpha = clamp(visibilityAlpha, 0.0, 1.0);
    }
    
    float finalAlpha = baseAlpha * visibilityAlpha;
    
    // Odrzucenie niewidocznych elementów
    if (finalAlpha < 0.01) {
        discard;
    }

    FragColor = vec4(baseColor, finalAlpha);
}