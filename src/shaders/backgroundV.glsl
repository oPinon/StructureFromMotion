#version 130

in vec4 vertex;
in vec2 texCoordsV;

out vec2 texCoords;

void main() {

	gl_Position = vertex;
	texCoords = texCoordsV;
}