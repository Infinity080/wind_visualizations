#version 430 core

float AMBIENT = 0.3;

uniform vec3 lightPos;
uniform vec3 cameraPos;
uniform float shininess = 15.0;
uniform sampler2D colorTexture;
uniform sampler2D normalMap;

in vec3 vecNormal;
in vec3 worldPos;
in vec2 texCoord;
in mat3 TBN;

out vec4 outColor;

void main()
{

    // Wektory oœwietlenia
    vec3 tangentNormal = texture(normalMap, texCoord).rgb;

    // Przekszta³cenie do zakresu [-1, 1]
    tangentNormal = normalize(tangentNormal * 2.0 - 1.0);

    // Przekszta³cenie normalnej z tangent space do world space za pomoc¹ TBN
    vec3 normal = normalize(TBN * tangentNormal);

    vec3 lightDir = normalize(lightPos - worldPos);
    vec3 viewDir  = normalize(cameraPos - worldPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    // Parametry oœwietlenia
    float diffuse = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess)*0.5f;

    // Tekstura
    vec3 surfaceColor = texture(colorTexture, vec2(texCoord.x, 1.0 - texCoord.y)).rgb;

    // Miksowanie oœwietlenia
    float lighting = clamp(AMBIENT + diffuse, 0.0, 1.0);

    // Wyjœciowy kolor
    outColor = vec4(surfaceColor * lighting * 4.0 + spec * vec3(1.0), 1.0);
}
