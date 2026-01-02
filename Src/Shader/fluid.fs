#version 330 core
out vec4 FragColour;
in vec2 texCoords;

uniform sampler2D densityTexture;

void main(){
float density = texture(densityTexture, texCoords).r;
density = clamp(density * 0.005, 0.0, 1.0);
FragColour = vec4(vec3(density), 1.0);
}