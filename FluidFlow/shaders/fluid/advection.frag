#version 330 core

out vec4 fragColor;

in vec2 texCoords;

uniform sampler2D screenTexture;

uniform float fps;

uniform float width;
uniform float height;

void main() {
	// delta t is 1/60
	// vec2 texCoords = vec2(0.5f * (coords.x + 1.0f), 0.5f * (coords.y + 1.0f));

	// vec2 newTexCoords = texCoords - (255 * texture(screenTexture, texCoords).xy / (60.0 * size));
	vec2 change = vec2((texture(screenTexture, texCoords).x)/(fps * width), (texture(screenTexture, texCoords).y)/(fps * height));
	vec2 newTexCoords = texCoords - change;
	fragColor = texture(screenTexture, newTexCoords);

	// fragColor = texture(screenTexture, texCoords);
}