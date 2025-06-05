#version 430 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;

uniform mat4 transformation;
uniform mat4 modelMatrix;

out vec3 worldNormal;
out vec3 worldPos;
out vec3 vecNormal;

void main() {
    vecNormal = vec3(modelMatrix * vec4(vertexNormal, 0.0));
    worldPos = (modelMatrix * vec4(vertexPosition, 1.0)).xyz;
    worldNormal = normalize((modelMatrix * vec4(vertexNormal, 0.0)).xyz);
    gl_Position = transformation * vec4(vertexPosition, 1.0);
}
