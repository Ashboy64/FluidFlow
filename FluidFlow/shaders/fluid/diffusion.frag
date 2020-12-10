#version 330 core

out vec4 fragColor;

in vec2 texCoords;

uniform sampler2D velocityTexture;

uniform float width;
uniform float height;

uniform float viscosity;
uniform float fps;

void main() {
	float offsetX = 1.0 / width;
	float offsetY = 1.0 / height;
	
	vec2 sum =	texture(velocityTexture, vec2(texCoords.x - offsetX, texCoords.y)).xy + 
				texture(velocityTexture, vec2(texCoords.x + offsetX, texCoords.y)).xy +
				texture(velocityTexture, vec2(texCoords.x, texCoords.y - offsetY)).xy +
				texture(velocityTexture, vec2(texCoords.x, texCoords.y + offsetY)).xy;
	
	float coeff = offsetX*offsetY / (viscosity*(1.0/fps));
	vec4 curr = texture(velocityTexture, texCoords);
	sum = sum + vec2(coeff * curr.x, coeff * curr.y);

	sum = vec2(sum.x/(4 + coeff), sum.y/(4 + coeff));

	fragColor = vec4(sum, curr.z, curr.w);
}