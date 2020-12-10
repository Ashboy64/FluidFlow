#version 330 core

out vec4 fragColor;
in vec2 texCoords;

uniform sampler2D inputTexture;

uniform float width;
uniform float height;

uniform bool velocity;

void main() {
	float offsetX = 1.0 / width;
	float offsetY = 1.0 / height;
	vec4 curr = texture(inputTexture, texCoords);

	float offsetToUse = 0.0;
	bool x = true;
	float multiplier = 1.0;

	float boundsX = offsetX;
	float boundsY = offsetY;

	if (texCoords.x <= boundsX) {
		if (velocity) {
			offsetToUse = offsetX;
			multiplier = -1.0;
		} else {
			offsetToUse = offsetX;
			multiplier = 1.0;
		}
	} else if (texCoords.x >= (1.0 - boundsX)) {
		if (velocity) {
			offsetToUse = -offsetX;
			multiplier = -1.0;
		} else {
			offsetToUse = -offsetX;
			multiplier = 1.0;
		}
	} else if (texCoords.y >= (1.0 - boundsY)) {
		if (velocity) {
			offsetToUse = -offsetY;
			multiplier = -1.0;
			x = false;
		} else {
			offsetToUse = -offsetY;
			multiplier = 1.0;
			x = false;
		}
	} else if (texCoords.y <= boundsY) {
		if (velocity) {
			offsetToUse = offsetY;
			multiplier = -1.0;
			x = false;
		} else {
			offsetToUse = offsetY;
			multiplier = 1.0;
			x = false;
		}
	}

	if (x) {
		curr = texture(inputTexture, vec2(texCoords.x + offsetToUse, texCoords.y));
		fragColor = vec4(multiplier * curr.x, multiplier * curr.y, curr.z, curr.w);
	} else {
		curr = texture(inputTexture, vec2(texCoords.x, texCoords.y + offsetToUse));
		fragColor = vec4(multiplier * curr.x, multiplier * curr.y, curr.z, curr.w);
	}
}