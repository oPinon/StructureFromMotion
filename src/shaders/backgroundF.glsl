#version 130

in vec2 texCoords;

uniform sampler2D texture;

out vec4 color;

void main() {

	color = 0.3*texture2D(texture,texCoords);
}