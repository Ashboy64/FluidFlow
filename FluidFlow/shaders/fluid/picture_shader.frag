#version 330 core

in vec2 texCoords;
out vec4 fragColor;

uniform sampler2D velocityTexture;
uniform sampler2D pictureTexture;

uniform float fps;

void main() {
	// delta t is 1/60
	vec2 change = vec2((texture(velocityTexture, texCoords).x)/(fps), (texture(velocityTexture, texCoords).y)/(fps));
	vec2 newTexCoords = texCoords - change;
	fragColor = texture(pictureTexture, newTexCoords);
}