#version 330 core

out vec4 fragColor;
in vec2 loc;

uniform float width;
uniform float height;

void main() {
	float mult = 2.0;
	float mag = 0.45; 
	fragColor = vec4(mag*(1000.0 / width)*sin(mult*3.1415*loc.y*height/1000.0), mag*(1000.0 / height)*sin(mult*3.1415*loc.x*width/1000.0), 0.0, 1.0);
	// fragColor = vec4(0.0, 0.0, 0.0, 1.0);
}