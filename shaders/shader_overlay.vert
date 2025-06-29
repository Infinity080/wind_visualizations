#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in float aWindSpeed;

out float WindSpeed;

uniform mat4 transformation;

void main()
{
    gl_Position = transformation * vec4(aPos, 1.0);
    WindSpeed = aWindSpeed;
}