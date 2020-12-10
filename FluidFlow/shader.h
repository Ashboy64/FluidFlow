#pragma once

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
public:
	// program id
	unsigned int ID;

	// constructor
	Shader();
	Shader(const char* vertexPath, const char* fragmentPath);

	// activate, calls useProgram
	void use();

	// functions to set uniform values
	void setBool(const std::string& name, bool value) const;
	void setInt(const std::string& name, int value) const;
	void setFloat(const std::string& name, float value) const;

private:
	void readCode(const char* vertexPath, const char* fragmentPath,
		std::string& vertexCode, std::string& fragmentCode);
	void checkShaderError(unsigned int shader, bool program = false);
};