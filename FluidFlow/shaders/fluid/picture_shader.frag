#version 330 core

in vec2 texCoords;
out vec4 fragColor;

uniform sampler2D velocityTexture;
uniform sampler2D pictureTexture;

uniform float fps;
uniform float width;
uniform float height;

void main() {
	// delta t is 1/60
	vec2 change = vec2((texture(velocityTexture, texCoords).x)/(fps * width), (texture(velocityTexture, texCoords).y)/(fps * height));
	vec2 newTexCoords = texCoords - change;
	fragColor = texture(pictureTexture, newTexCoords);
}