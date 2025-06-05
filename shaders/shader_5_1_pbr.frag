#version 430 core

// Uniformy tekstur
uniform sampler2D colorTexture;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D emissionMap; // Mapa emisji

// Uniformy dla PBR
uniform float roughness; // Wartoœæ skaluj¹ca pobrana z roughnessMap (dla statku)
uniform float metallic;  // Wartoœæ skaluj¹ca pobrana z metallicMap (dla statku)

// Uniformy sceniczne
uniform vec3 lightPos;
uniform vec3 cameraPos;

// Zmienne z vertex shadera
in vec3 worldPos;
in vec2 texCoord;
in mat3 TBN;

// Wyjœcie
out vec4 outColor;

// Sta³a PI
const float PI = 3.14159265359;

// Funkcja rozk³adu normalnego GGX
float DistributionGGX(vec3 N, vec3 H, float a)
{
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float nom    = a2;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    denom        = PI * denom * denom;
    return nom / denom;
}

// Funkcja geometrii Schlick-GGX dla pojedynczego kierunku
float GeometrySchlickGGX(float NdotV, float k)
{
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

// Funkcja geometrii Smitha
float GeometrySmith(vec3 N, vec3 V, vec3 L, float a)
{
    float k = (a + 1.0) * (a + 1.0) / 8.0;
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);
    return ggx1 * ggx2;
}

// Przybli¿enie Fresnela - Schlick
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    // Obliczenie kierunku widoku
    vec3 V = normalize(cameraPos - worldPos);

    // Pobranie albedo z tekstury
    vec3 albedo = texture(colorTexture, texCoord).rgb;

    // Pobranie i przeskalowanie roughness oraz metalicznoœci z tekstur
    float roughnessMapValue = texture(roughnessMap, texCoord).r;
    float metallicMapValue  = texture(metallicMap, texCoord).r;
    float roughnessValue = roughnessMapValue * roughness;
    float metallicValue  = metallicMapValue * metallic;

    // Pobranie normalnej z mapy normalnych i przekszta³cenie do przestrzeni œwiata
    vec3 tangentNormal = texture(normalMap, texCoord).rgb;
    tangentNormal = normalize(tangentNormal * 2.0 - 1.0);
    vec3 N = normalize(TBN * tangentNormal);

    // Kierunki œwiat³a i po³ysku
    vec3 L = normalize(lightPos - worldPos);
    vec3 H = normalize(V + L);

    // Obliczenie podstawowych wartoœci dot product
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    // Przybli¿enie Fresnela: F0 = 0.04, z modyfikacj¹ dla metalicznych.
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallicValue);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    float D = DistributionGGX(N, H, roughnessValue);
    float G = GeometrySmith(N, V, L, roughnessValue);

    vec3 numerator = D * G * F;
    float denominator = 4.0 * max(NdotL, 0.0) * max(NdotV, 0.0) + 0.001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallicValue;

    vec3 diffuse = (kD * albedo / PI);
    vec3 pbrColor = (diffuse + specular) * NdotL;

    // Dodajemy ambient, aby scena by³a jaœniejsza
    vec3 ambient = 0.5 * albedo;
    vec3 combinedPBR = ambient + pbrColor;

    // Pobranie emisji z mapy emisji – efekt œwiecenia silników
    vec3 emission = texture(emissionMap, texCoord).rgb;

    // Finalny kolor = PBR + emisja (emisja bez dodatkowego modelu oœwietlenia)
    vec3 finalColor = combinedPBR + emission;

    outColor = vec4(finalColor, 1.0);
}