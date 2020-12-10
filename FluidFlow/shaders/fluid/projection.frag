#version 330 core

out vec4 fragColor;
in vec2 texCoords;

uniform sampler2D pressureTexture;
uniform sampler2D velocityTexture;

uniform float width;
uniform float height;

void main() {
	float offsetX = 1.0 / width;
	float offsetY = 1.0 / height;
	
	float gX =	texture(pressureTexture, vec2(texCoords.x + offsetX, texCoords.y)).x -
				texture(pressureTexture, vec2(texCoords.x - offsetX, texCoords.y)).x;
	float gY =	texture(pressureTexture, vec2(texCoords.x, texCoords.y + offsetY)).x -
				texture(pressureTexture, vec2(texCoords.x, texCoords.y - offsetY)).x;
	
	gX = gX / (2.0 * offsetX);
	gY = gY / (2.0 * offsetY);

	vec4 curr = texture(velocityTexture, texCoords);
	fragColor = vec4(curr.x - gX, curr.y - gY, curr.z, curr.w);
}