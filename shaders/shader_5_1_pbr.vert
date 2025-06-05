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
out vec3 worldNormal;
out vec2 texCoord;
out mat3 TBN; // Tangent-Bitangent-Normal matrix to fragment shader

void main()
{
    // Transform positions and vectors to world space
    vec4 world = modelMatrix * vec4(vertexPosition, 1.0);
    worldPos = world.xyz;

    vecNormal =  vec3(modelMatrix * vec4(vertexNormal, 0.0));

    vec3 T = normalize((modelMatrix * vec4(vertexTangent, 0.0)).xyz);
    vec3 B = normalize((modelMatrix * vec4(vertexBitangent, 0.0)).xyz);
    vec3 N = normalize((modelMatrix * vec4(vertexNormal, 0.0)).xyz);

    TBN = mat3(T, B, N);
    
    worldNormal = N;
    texCoord = vertexTexCoord;
    gl_Position = transformation * vec4(vertexPosition, 1.0);
}