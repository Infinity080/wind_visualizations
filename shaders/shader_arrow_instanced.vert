#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

layout (location = 3) in mat4 modelMatrix;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 viewProjectionMatrix;

void main()
{
    FragPos = vec3(modelMatrix * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(modelMatrix))) * aNormal;

    gl_Position = viewProjectionMatrix * vec4(FragPos, 1.0);
}