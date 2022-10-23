#version 440 core
/*out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2DArray ourTexture;

void main(){
    FragColor = texture(ourTexture, vec3(TexCoord, 2));
}*/
out vec4 FragColor;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;    
    float shininess;
}; 
struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
in vec3 Normal;
in vec3 Position;

uniform vec3 cameraPos;
uniform samplerCube skybox;
uniform Material material;
uniform Light light;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;



void main()
{    
   // ambient
    vec3 ambient = light.ambient * material.ambient;
  	//vec3 ambient = light.ambient * 0.2f;
    // diffuse 
	vec3 norm = Normal;
    vec3 lightDir = normalize(light.position - Position);
    //float diff = dot(norm, lightDir);
	float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * (diff * material.diffuse);
    
    // specular
    vec3 viewDir = normalize(cameraPos - Position);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * material.specular);  
        
    vec3 light_result = ambient + diffuse + specular;
	//vec3 light_result = ambient + diffuse;

	vec3 Inject = -viewDir;
	vec3 R = reflect(Inject, norm);
	vec3 Reflect = texture(skybox, R).rgb;

	
	float fresnel = 0.02 + 0.98 * pow(1.0 - abs(dot(norm, Inject)), 5.0);
    FragColor = vec4(light_result * mix(material.ambient, Reflect, fresnel), 1.0);
}