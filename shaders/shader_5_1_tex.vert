#version 430 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;
layout(location = 3) in vec3 vertexTangent;
layout(location = 4) in vec3 vertexBitangent;

uniform mat4 transformation;
uniform mat4 modelMatrix;

out vec3 vecNormal;
out vec3 worldPos;
out vec2 texCoord;
out mat3 TBN;

void main()
{
    vec3 T = normalize(mat3(modelMatrix) * vertexTangent);
    vec3 B = normalize(mat3(modelMatrix) * vertexBitangent);
    vec3 N_vert = normalize(mat3(modelMatrix) * vertexNormal);

    TBN = mat3(T, B, N_vert);

    texCoord = vertexTexCoord;
    vec4 world = modelMatrix * vec4(vertexPosition, 1.0);
    worldPos = world.xyz;

    vecNormal =  vec3(modelMatrix * vec4(vertexNormal, 0.0));

    gl_Position = transformation * vec4(vertexPosition, 1.0);
}