#version 410 core
out vec4 FragColor;

in float WindSpeed;

uniform float maxWindSpeed;

void main()
{
    float t = clamp(WindSpeed / maxWindSpeed, 0.0, 1.0);
    vec3 blue = vec3(0.0, 0.0, 1.0);
    vec3 green = vec3(0.0, 1.0, 0.0);
    vec3 color = mix(blue, green, t);
    FragColor = vec4(color, 0.5); // RGBA - 4. parametr to przezroczystoœæ
}