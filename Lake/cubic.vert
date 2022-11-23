#version 440 core

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