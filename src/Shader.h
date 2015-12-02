#pragma once

#include <GL/glew.h>
#include <GL/glut.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;

string readFile(const string& fileName) {

	ifstream in(fileName, ios::binary | ios::ate);
	if (!in.is_open()) { cerr << "Error, can't read " << fileName.c_str() << endl; throw 1; }
	std::streamsize size = in.tellg();
	in.seekg(0, ios::beg);
	vector<char> buffer(size);
	in.read(buffer.data(), size);
	return string(buffer.data(), buffer.size());
}

struct Shader {

	GLuint vert, frag, prog;
	string name;

	static void displayShaderErrors(GLuint shader, const char* header) {
		int logLength = 0;
		int charsWritten = 0;
		char *log;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength > 1)
		{
			cout << header << endl;
			log = (char *)malloc(logLength);
			glGetShaderInfoLog(shader, logLength, &charsWritten, log);
			cout << log << endl;
			free(log);
			throw 1;
		}
	}

	Shader() { }
	Shader(const string& vertFile, const string& fragFile) : name(vertFile + " " + fragFile) {

		string vertCode = readFile(vertFile).data();
		const char* vertCodeP = vertCode.data();
		vert = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vert, 1, &vertCodeP, NULL);
		glCompileShader(vert);
		displayShaderErrors(vert, vertFile.c_str());

		string fragCode = readFile(fragFile).data();
		const char* fragCodeP = fragCode.data();
		frag = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(frag, 1, &fragCodeP, NULL);
		glCompileShader(frag);
		displayShaderErrors(frag, fragFile.c_str());

		prog = glCreateProgram();
		glAttachShader(prog, vert);
		glAttachShader(prog, frag);
		glLinkProgram(prog);
		int logLength;
		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength > 0) {
			char* log = (char*)malloc(logLength);
			glGetProgramInfoLog(prog, logLength, NULL, log);
			cout << log << endl;
			free(log);
			throw 1;
		}

		cout << "compiled and linked " << name.c_str() << endl;
	}
	GLint getUniformLocation(const char* varName) {
		GLint pos = glGetUniformLocation(prog, varName);
		if (pos == -1) {
			cerr << "cannot find uniform " << varName << " in " << name.c_str() << endl;
		}
		return pos;
	}
	GLint getAttribLocation(const char* varName) {
		GLint pos = glGetAttribLocation(prog, varName);
		if (pos == -1) {
			cerr << "cannot find attribute " << varName << " in " << name.c_str() << endl;
		}
		return pos;
	}
	void use() {
		glUseProgram(prog);
	}
};