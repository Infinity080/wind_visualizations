#version 430 core

in vec3 worldPos;
in vec3 worldNormal;
in vec2 texCoord;

out vec4 fragColor;

uniform vec3 planetCenter;
uniform vec3 lightPos;
uniform vec3 cameraPos;

uniform sampler2D cloudColor;
uniform sampler2D cloudAlpha;



void main() {
    vec3 dir = normalize(worldPos - planetCenter);


    vec2 uv = texCoord;

    vec4 texColor = vec4(texture(cloudColor, uv).rgb, 1.0);

    vec3 sunDir = normalize(lightPos - worldPos);
    float sunInfluence = clamp(dot(worldNormal, sunDir), 0.0, 1.0);
    texColor *= sunInfluence;

    vec3 viewDir = normalize(cameraPos - worldPos);
    float dotVN = dot(viewDir, worldNormal);
    float rim = 1.0 - dotVN; // 0 in center, ~1 at limb
    float glow = pow(clamp(rim, 0.0, 1.0), 1.5); // you can tweak exponent here

    vec3 cloudTex = texture(cloudColor, uv).rgb;
    vec3 color = mix(cloudTex, vec3(1.0), 0.0); 
    float alpha = (1-texture(cloudAlpha, uv).r)*2.0;


    float rimWeight = pow(clamp(rim, 0.0, 1.0), 1.5);
    float rimBias = 0.3; // base visibility even at center
    float finalAlpha = alpha * (rimBias + (1.0 - rimBias) * rimWeight) * sunInfluence;



    fragColor = vec4(color, finalAlpha);
}

/*

#version 430 core

in vec3 worldPos;
in vec3 worldNormal;
in vec2 texCoord;

out vec4 fragColor;

uniform vec3 planetCenter;
uniform vec3 lightPos;
uniform vec3 cameraPos;

uniform sampler2D cloudColor;
uniform sampler2D cloudAlpha;

void main() {
    vec2 uv = texCoord;

    // Directions
    vec3 sunDir = normalize(lightPos - worldPos);
    vec3 viewDir = normalize(cameraPos - worldPos);
    vec3 normal = normalize(worldNormal);

    // Basic lighting and fresnel
    float diffuse = clamp(dot(normal, sunDir), 0.0, 1.0);
    float fresnel = pow(1.0 - dot(viewDir, normal), 2.0); // Rim highlight
    float rim = 1.0 - dot(viewDir, normal); // For softer alpha
    float rimWeight = pow(clamp(rim, 0.0, 1.0), 1.3);
    float rimBias = 0.35;

    // Sample textures
    vec3 cloudTex = texture(cloudColor, uv).rgb;
    float alpha = 1.0-texture(cloudAlpha, uv).r;

    // Shadow/occlusion trick (optional)
    float selfShadow = pow(1.0 - alpha, 2.0);
    vec3 cloudColor = mix(vec3(0.8), vec3(1.0), selfShadow); // depth gradient
    cloudColor = mix(cloudColor, cloudTex, 0.4);              // combine with texture
    cloudColor *= 0.4 + 0.6 * diffuse;                        // sunlight influence

    // Final alpha (includes rim and base visibility)
    float finalAlpha = alpha * (rimBias + (1.0 - rimBias) * rimWeight);

    fragColor = vec4(cloudColor, finalAlpha);
}*/