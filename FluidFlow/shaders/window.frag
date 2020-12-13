#version 330 core

out vec4 fragColor;

in vec2 texCoords;

uniform sampler2D screenTexture;
uniform bool swappingMain;

void main() {
	vec4 curr = texture(screenTexture, texCoords);
	if (swappingMain) {
		fragColor = vec4((5) * curr.x, (5) * curr.y, curr.z, curr.w);
	} else {
		fragColor = vec4(curr.x, curr.y, curr.z, curr.w);
	}
}