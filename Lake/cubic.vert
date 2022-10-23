#version 440 core
/*layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aColor; // input color
layout (location = 3) in vec3 aNormalCoord; // normal


layout (binding = 1, rgba32f) uniform image2DArray tex1;
layout (binding = 2, rgba32f) uniform image2DArray tex2;
layout (binding = 3, rgba32f) uniform image2DArray tex3;

layout (std140, binding = 0) uniform Matrix { // Model View Projection matrices
    mat4 model;
	mat4 modelCube;
    mat4 view;
    mat4 viewCube;
    mat4 proj;
	vec3 cameraPosition;
} ubo;

layout (std140, binding = 2) uniform Light{ // values of the material related to lighting
    float specularHighlight;
	vec3 ambientColor;
	vec3 diffuseColor;
	vec3 specularColor;
	vec3 emissiveColor;
	vec4 lightPosition;
} ubl;
uniform int N = 512;

layout (location = 0) out vec2 TexCoord;
layout (location = 1) out vec3 fragAmbientColor;
layout (location = 2) out vec3 fragDiffuseColor;
layout (location = 3) out vec4 fragLightVector;
layout (location = 4) out vec3 fragVectorFromCameraToPixel;
layout (location = 5) out vec3 fragNormal;
layout (location = 6) out vec3 fragLightPos;
layout (location = 7) out vec3 fragWorldPixelToCamera;

layout (location = 8) out vec3 fragColor; // variables that will be passed to the fragment shader
layout (location = 9) out vec2 fragTexCoord;
out vec4 fragNormalCoord;

void main(){
    vec4 textureSampledHeightMap = imageLoad(tex1, ivec3(N * aTexCoord, 0));
	vec4 textureSampledDispX = imageLoad(tex3, ivec3(N * aTexCoord, 0));
	vec4 textureSampledDispZ = imageLoad(tex3, ivec3(N * aTexCoord, 1));
	vec4 textureSampledSlopeX = imageLoad(tex3, ivec3(N * aTexCoord, 2));
	vec4 textureSamledSlopeZ = imageLoad(tex3, ivec3(N * aTexCoord, 3));

	vec4 WCS_position; // choppy wave
	WCS_position = ubo.model * vec4(vec3(aPos.x + textureSampledDispX.x , aPos.y + textureSampledHeightMap.x, aPos.z + textureSampledDispZ.x ), 1.0);
	vec4 VCS_position = ubo.view * WCS_position; // Position in VCS
	gl_Position = ubo.proj * VCS_position; // Position in NVCS
	fragColor = aColor; // base color of the triangle
	fragTexCoord = aTexCoord; // texture coordinates
	
	vec3 normalCoords = normalize(vec3(textureSampledSlopeX.x, 1.0f, textureSamledSlopeZ.x)); // normalize normal vectors

	fragNormalCoord = ubo.view * ubo.model * vec4(normalCoords, 0.0); // normal in VCS
	fragLightVector = ubo.view * ubl.lightPosition - VCS_position; // vector from the pixel to the light
	fragAmbientColor = ubl.ambientColor; // ambient light parameters
	fragDiffuseColor = ubl.diffuseColor; // diffuse light parameters
	fragVectorFromCameraToPixel = normalize(aPos - ubo.cameraPosition); // vector from the camera to the pixel in WCS
    fragNormal = normalCoords; // normal coordinates in WCS
    fragLightPos = ubl.lightPosition.xyz; // light position in WCS
	fragWorldPixelToCamera = normalize(ubo.cameraPosition - aPos); // vector from the pixel to the camera in WCS
}*/
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (binding = 1, rgba32f) uniform image2DArray tex1;
layout (binding = 3, rgba32f) uniform image2DArray tex3;

int N = 256;
out vec3 Normal;
out vec3 Position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


void main()
{
    vec4 textureSampledHeightMap = imageLoad(tex1, ivec3(N * aTexCoord, 0));
	vec4 textureSampledDispX = imageLoad(tex3, ivec3(N * aTexCoord, 0));
	vec4 textureSampledDispZ = imageLoad(tex3, ivec3(N * aTexCoord, 1));
	vec4 textureSampledSlopeX = imageLoad(tex3, ivec3(N * aTexCoord, 2));
    vec4 textureSampledSlopeZ = imageLoad(tex3, ivec3(N * aTexCoord, 3));
    
    Normal = normalize(vec3(textureSampledSlopeX.x, 1.0f, textureSampledSlopeZ.x));
	
	
	Position = vec3(model * vec4(aPos.x + textureSampledDispX.x, aPos.y + textureSampledHeightMap.x, aPos.z + textureSampledDispZ.x, 1.0));
    gl_Position = projection * view * vec4(Position, 1.0);
}