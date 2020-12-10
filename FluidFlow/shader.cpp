#include "shader.h"

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
	// first load code from file
	std::string vertexCode;
	std::string fragmentCode;

	readCode(vertexPath, fragmentPath, vertexCode, fragmentCode);

	const char* vShaderCode = vertexCode.c_str();
	const char* fShaderCode = fragmentCode.c_str();

	// compile and link shaders
	unsigned int vertex, fragment;

	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vShaderCode, NULL);
	glCompileShader(vertex);
	checkShaderError(vertex);

	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fShaderCode, NULL);
	glCompileShader(fragment);
	checkShaderError(fragment);

	ID = glCreateProgram();
	glAttachShader(ID, vertex);
	glAttachShader(ID, fragment);
	glLinkProgram(ID);
	checkShaderError(ID, true);

	glDeleteShader(vertex);
	glDeleteShader(fragment);
}

Shader::Shader() {}


void Shader::use() {
	glUseProgram(ID);
}


void Shader::setBool(const std::string& name, bool value) const {
	glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}


void Shader::setInt(const std::string& name, int value) const {
	glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}


void Shader::setFloat(const std::string& name, float value) const {
	glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}


void Shader::checkShaderError(unsigned int shader, bool program) {
	int success;
	char infoLog[512];

	if (program) {
		glGetProgramiv(shader, GL_COMPILE_STATUS, &success);
	}
	else {
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	}

	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		std::cout << "ERROR COMPILING SHADER: " << infoLog << std::endl;
	}
}


void Shader::readCode(const char* vertexPath, const char* fragmentPath,
	std::string& vertexCode, std::string& fragmentCode) {

	std::ifstream vertexStream(vertexPath);
	std::ifstream fragmentStream(fragmentPath);

	std::string line;

	if (vertexStream.is_open()) {
		while (std::getline(vertexStream, line)) {
			vertexCode += line + '\n';
		}
	}
	else {
		std::cout << "Unable to open vertex shader file" << std::endl;
	}

	if (fragmentStream.is_open()) {
		while (std::getline(fragmentStream, line)) {
			fragmentCode += line + '\n';
		}
	}
	else {
		std::cout << "Unable to open fragment shader file" << std::endl;
	}
}