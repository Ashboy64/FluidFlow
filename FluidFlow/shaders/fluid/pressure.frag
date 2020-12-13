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


	// first compute divergence of velocity field
	float x_term =	texture(velocityTexture, vec2(texCoords.x + offsetX, texCoords.y)).x - 
					texture(velocityTexture, vec2(texCoords.x - offsetX, texCoords.y)).x;
	x_term = x_term / (2.0 * 1);

	float y_term =	texture(velocityTexture, vec2(texCoords.x, texCoords.y + offsetY)).y - 
					texture(velocityTexture, vec2(texCoords.x, texCoords.y - offsetY)).y;
	y_term = y_term / (2.0 * 1);

	float vel_divergence = x_term + y_term;

	// now perform jacobi iteration to solve for new pressure field
	float sum =	texture(pressureTexture, vec2(texCoords.x - offsetX, texCoords.y)).x + 
				texture(pressureTexture, vec2(texCoords.x + offsetX, texCoords.y)).x +
				texture(pressureTexture, vec2(texCoords.x, texCoords.y - offsetY)).x +
				texture(pressureTexture, vec2(texCoords.x, texCoords.y + offsetY)).x;
	
	float coeff = -1.0 * 1 * vel_divergence;
	float pressure = (sum + coeff) / 4.0;

	fragColor = vec4(pressure, 0.0, 0.0, 1.0);
}