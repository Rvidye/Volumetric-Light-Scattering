#version 460 core

in vec2 out_texcoord;
in vec3 Normal;
in vec3 Pos;

struct Material
{
    vec3 diffuseMat;
    vec3 specularMat;
    float shininess;
    float opacity;
};

struct Light {
	vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform vec3 viewPos;
uniform Material material;
uniform Light light;

out vec4 FragColor;

vec4 directionalLight(Light light, Material material,vec3 normal, vec3 viewDir)
{
	vec3 ambient = light.ambient * material.diffuseMat;

	vec3 lightDir = normalize(-light.direction);
	vec3 diffuse = light.diffuse * max(dot(normal,lightDir),0.0) * material.diffuseMat;

	vec3 reflectVec = reflect(-lightDir, normal);
    vec3 specular = light.specular * pow(max(dot(viewDir, reflectVec),0.0), material.shininess) * material.specularMat;
	return vec4((ambient+diffuse+specular),material.opacity);
}

vec4 pointLight(Light light, Material material,vec3 normal, vec3 viewDir)
{
	vec3 ambient = light.ambient * material.diffuseMat;

	vec3 lightDir = normalize(light.direction - Pos);
	vec3 diffuse = light.diffuse * max(dot(normal,lightDir),0.0) * material.diffuseMat;

	vec3 reflectVec = reflect(-lightDir, normal);
    vec3 specular = light.specular * pow(max(dot(viewDir, reflectVec),0.0), material.shininess) * material.specularMat;
	// attenuation
    float dist = length(light.direction - Pos);
    // attenuation = constant + linear * distance + quadratic * distance * distance
    float attenuation = 1.0 / (1.0 + 0.14 * dist + 0.07 * (dist * dist));

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

	return vec4((ambient+diffuse+specular),material.opacity);
}

void main(void)
{
	vec3 N = normalize(Normal);
	vec3 viewDir = normalize(viewPos - Pos);
	FragColor = directionalLight(light,material,N,viewDir);
}
