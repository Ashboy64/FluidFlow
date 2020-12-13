#version 330 core

out vec4 fragColor;
in vec2 texCoords;
in vec2 windowCoords;

uniform sampler2D velocityTexture;

uniform float width;
uniform float height;

uniform float fps;

uniform float magnitude_x;
uniform float magnitude_y;
uniform float xPos;
uniform float yPos;

void main() {

	float distX = windowCoords.x - xPos;
	float distY = windowCoords.y - yPos;
	float dist_s = (distX*distX) + (distY*distY);

	float coeff = exp(-1.0 * dist_s / (0.001));
	vec2 change = vec2(coeff * magnitude_x, coeff * magnitude_y);

	vec4 curr = texture(velocityTexture, texCoords);
	fragColor = vec4(curr.xy + change, 0.0, 1.0);
}