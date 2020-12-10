#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <tgmath.h>
#include <direct.h>

#include "shader.h"
#include "simulation.h"

Simulation::Simulation() {
	// GLFW provides basic functionality to define an OpenGL context and application window
	initGLFW();

	width = 1000;
	height = 1000;

	// create window and set gl context
	window = createWindow();
	glfwMakeContextCurrent(window);


	/* Checks if GLAD is initialized. GLAD fetches the actual implementations of the OpenGL functions used
	from the appropriate places.*/
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
	}

	glViewport(0, 0, width, height);

	// sets framebufferSizeCallback to be called when window resized
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	// load shaders
	shader = Shader("shaders/vertex_shader.vert", "shaders/fragment_shader.frag");	// draws on picture texture
	backgroundShader = Shader("shaders/background_shader.vert", "shaders/background_shader.frag");	// background for the picture texture
	pictureShader = Shader("shaders/fluid/picture_shader.vert", "shaders/fluid/picture_shader.frag");	// computes new picture from vel field
	screenShader = Shader("shaders/window.vert", "shaders/window.frag");	// puts resulting picture on the actual window

	initialVField = Shader("shaders/fluid/initial_vfield.vert", "shaders/fluid/initial_vfield.frag");	// initial v field
	advectionShader = Shader("shaders/fluid/advection.vert", "shaders/fluid/advection.frag");		// advection for vel field
	diffusionShader = Shader("shaders/fluid/diffusion.vert", "shaders/fluid/diffusion.frag");		// viscous diffusion
	force_shader = Shader("shaders/fluid/force_shader.vert", "shaders/fluid/force_shader.frag");		// external forces
	pressureShader = Shader("shaders/fluid/pressure.vert", "shaders/fluid/pressure.frag");		// solves for pressure field
	projectionShader = Shader("shaders/fluid/projection.vert", "shaders/fluid/projection.frag");		// subtracts grad pressure field
	boundaryShader = Shader("shaders/fluid/boundary.vert", "shaders/fluid/boundary.frag");		// subtracts grad pressure field


	loadFramebuffers();

	screenVAO = windowQuad();
	background = sceneBackground();

	drawInitialPicture();
	drawInitialVelField();
	drawInitialPressureField();
}


void Simulation::run() {
	while (!glfwWindowShouldClose(window)) {
		processInput(window);
		glfwGetWindowSize(window, &width, &height);

		double x, y;
		glfwGetCursorPos(window, &x, &y);
		forceX = (float)x;
		forceY = (float)y;

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
			previousX = forceX;
			previousY = forceY;
		}

		advection();
		diffusion();
		forceApplication();
		pressureSolve();
		projectToDivergenceFree();
		boundaryConditions();

		newImage();
		swapToMain();
	}
}


void Simulation::advection() {
	glBindFramebuffer(GL_FRAMEBUFFER, intermediateVelocityFramebuffer);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, velocityTexture);

	// advection
	glBindVertexArray(screenVAO);
	advectionShader.use();
	advectionShader.setFloat("fps", millisecondsPerFrame);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);	// unbinding VAO

	swapBuffers(intermediateVelocityTexture, velocityFramebuffer);
}


void Simulation::diffusion() {
	glBindFramebuffer(GL_FRAMEBUFFER, intermediateVelocityFramebuffer);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, velocityTexture);
	glBindVertexArray(screenVAO);

	diffusionShader.use();
	diffusionShader.setFloat("width", width);
	diffusionShader.setFloat("height", height);
	diffusionShader.setFloat("viscosity", 1.0f / 100.0f);
	diffusionShader.setInt("velocityTexture", 0);
	diffusionShader.setFloat("fps", millisecondsPerFrame);

	for (int i = 0; i < 40; i++) {
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}

	glBindVertexArray(0);	// unbinding VAO

	swapBuffers(intermediateVelocityTexture, velocityFramebuffer);
}


void Simulation::forceApplication() {
	glBindFramebuffer(GL_FRAMEBUFFER, intermediateVelocityFramebuffer);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, velocityTexture);
	glBindVertexArray(screenVAO);

	force_shader.use();
	force_shader.setInt("velocityTexture", 0);
	force_shader.setFloat("width", width);
	force_shader.setFloat("height", height);
	force_shader.setFloat("fps", millisecondsPerFrame);
	force_shader.setFloat("magnitude_x", 1.0f * (forceX - previousX));
	force_shader.setFloat("magnitude_y", -1.0f * (forceY - previousY));
	force_shader.setFloat("xPos", previousX);
	force_shader.setFloat("yPos", previousY);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	swapBuffers(intermediateVelocityTexture, velocityFramebuffer);
}


void Simulation::pressureSolve() {
	glBindFramebuffer(GL_FRAMEBUFFER, intermediatePressureFramebuffer);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pressureTexture);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, velocityTexture);

	glBindVertexArray(screenVAO);

	pressureShader.use();
	pressureShader.setInt("pressureTexture", 0);
	pressureShader.setInt("velocityTexture", 1);
	pressureShader.setFloat("width", width);
	pressureShader.setFloat("height", height);

	for (int i = 0; i < 40; i++) {
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}

	glBindVertexArray(0);

	swapBuffers(intermediatePressureTexture, pressureFramebuffer);
}


void Simulation::projectToDivergenceFree() {
	glBindFramebuffer(GL_FRAMEBUFFER, intermediateVelocityFramebuffer);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pressureTexture);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, velocityTexture);

	glBindVertexArray(screenVAO);

	projectionShader.use();
	projectionShader.setInt("pressureTexture", 0);
	projectionShader.setInt("velocityTexture", 1);
	projectionShader.setFloat("width", width);
	projectionShader.setFloat("height", height);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);

	swapBuffers(intermediateVelocityTexture, velocityFramebuffer);
}


void Simulation::boundaryConditions() {
	glBindFramebuffer(GL_FRAMEBUFFER, intermediateVelocityFramebuffer);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pressureTexture);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, velocityTexture);

	glBindVertexArray(screenVAO);

	boundaryShader.use();
	boundaryShader.setInt("inputTexture", 1);
	boundaryShader.setFloat("width", width);
	boundaryShader.setFloat("height", height);
	boundaryShader.setBool("velocity", true);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, intermediatePressureFramebuffer);
	boundaryShader.setInt("inputTexture", 0);
	boundaryShader.setBool("velocity", false);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
	
	swapBuffers(intermediateVelocityTexture, velocityFramebuffer);
	swapBuffers(intermediatePressureTexture, pressureFramebuffer);
}


void Simulation::newImage() {
	glBindFramebuffer(GL_FRAMEBUFFER, intermediatePictureFramebuffer);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, velocityTexture);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, pictureTexture);

	glBindVertexArray(screenVAO);
	pictureShader.use();
	pictureShader.setInt("velocityTexture", 0);
	pictureShader.setInt("pictureTexture", 1);
	pictureShader.setFloat("fps", millisecondsPerFrame);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);	// unbinding VAO

	swapBuffers(intermediatePictureTexture, pictureFramebuffer);
}


void Simulation::swapToMain() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	screenShader.use();

	glBindVertexArray(screenVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pictureTexture);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glfwSwapBuffers(window);	// show current buffer on screen
	glfwPollEvents();	// check if any inputs triggered aand update window state
}


void Simulation::swapBuffers(unsigned int sourceTexture, unsigned int targetFramebuffer) {
	glBindFramebuffer(GL_FRAMEBUFFER, targetFramebuffer);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sourceTexture);
	glBindVertexArray(screenVAO);

	screenShader.use();
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}


void Simulation::drawInitialPressureField() {
	for (unsigned int i : {0, 1}) {
		unsigned int framebuffer;
		unsigned int texture;
		if (i == 0) {
			framebuffer = pressureFramebuffer;
			texture = pressureTexture;
		}
		else {
			framebuffer = intermediatePressureFramebuffer;
			texture = intermediatePressureTexture;
		}

		// draw on the velocity framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glBindTexture(GL_TEXTURE_2D, texture);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		backgroundShader.use();
		glBindVertexArray(background);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}
}


void Simulation::drawInitialVelField() {
	for (unsigned int i : {0, 1}) {
		unsigned int framebuffer;
		unsigned int texture;
		if (i == 0) {
			framebuffer = velocityFramebuffer;
			texture = velocityTexture;
		}
		else {
			framebuffer = intermediateVelocityFramebuffer;
			texture = intermediateVelocityTexture;
		}

		// draw on the velocity framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glBindTexture(GL_TEXTURE_2D, texture);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		initialVField.use();
		initialVField.setFloat("width", (float)width);
		initialVField.setFloat("height", (float)height);
		glBindVertexArray(initialVelocityField());
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}
}


void Simulation::drawInitialPicture() {
	for (unsigned int i : {0, 1}) {
		unsigned int framebuffer;
		unsigned int texture;
		if (i == 0) {
			framebuffer = pictureFramebuffer;
			texture = pictureTexture;
		}
		else {
			framebuffer = intermediatePictureFramebuffer;
			texture = intermediatePictureTexture;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glBindTexture(GL_TEXTURE_2D, texture);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		backgroundShader.use();
		glBindVertexArray(background);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		shader.use();
		glBindVertexArray(sceneData());
		glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
	}
}


unsigned int Simulation::initialVelocityField() {

	float val = 0.01f;

	float screenVertices[] = {
		1.0f, 1.0f, 0.0f,	 val, val, 0.0f,
		1.0f, -1.0f, 0.0f,	 val, val, 0.0f,
		-1.0f, -1.0f, 0.0f,	 val, val, 0.0f,
		-1.0f, 1.0f, 0.0f,	 val, val, 0.0f
	};

	unsigned int screenIndices[] = {
		0, 1, 3,
		1, 2, 3
	};

	unsigned int screenVAO;
	glGenVertexArrays(1, &screenVAO);

	unsigned int screenVBO;
	glGenBuffers(1, &screenVBO);

	unsigned int screenEBO;
	glGenBuffers(1, &screenEBO);

	glBindVertexArray(screenVAO);
	glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, screenEBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(screenVertices), screenVertices, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(screenIndices), screenIndices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	return screenVAO;
}


unsigned int Simulation::windowQuad() {
	float screenVertices[] = {
		1.0f, 1.0f, 0.0f,	 1.0f, 1.0f,
		1.0f, -1.0f, 0.0f,	 1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f,	 0.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,	 0.0f, 1.0f
	};

	unsigned int screenIndices[] = {
		0, 1, 3,
		1, 2, 3
	};

	unsigned int screenVAO;
	glGenVertexArrays(1, &screenVAO);

	unsigned int screenVBO;
	glGenBuffers(1, &screenVBO);

	unsigned int screenEBO;
	glGenBuffers(1, &screenEBO);

	glBindVertexArray(screenVAO);
	glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, screenEBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(screenVertices), screenVertices, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(screenIndices), screenIndices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	return screenVAO;
}


unsigned int Simulation::sceneBackground() {
	float screenVertices[] = {
		1.0f, 1.0f, 0.0f,	 0.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 0.0f,	 0.0f, 0.0f, 0.0f,
		-1.0f, -1.0f, 0.0f,	 0.0f, 0.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,	 0.0f, 0.0f, 0.0f
	};

	unsigned int screenIndices[] = {
		0, 1, 3,
		1, 2, 3
	};

	unsigned int screenVAO;
	glGenVertexArrays(1, &screenVAO);

	unsigned int screenVBO;
	glGenBuffers(1, &screenVBO);

	unsigned int screenEBO;
	glGenBuffers(1, &screenEBO);

	glBindVertexArray(screenVAO);
	glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, screenEBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(screenVertices), screenVertices, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(screenIndices), screenIndices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	return screenVAO;
}

unsigned int Simulation::sceneData() {
	float offsetX = 0.50f * 1000.0f / width;
	float offsetY = 0.50f * 1000.0f / height;

	float vertices[] = {
		-offsetX, -offsetY, 0.0f,
		offsetX, -offsetY, 0.0f,
		0.0f, offsetY, 0.0f
	};

	unsigned int indices[] = {
		0, 1, 2
	};

	unsigned int VAO;
	glGenVertexArrays(1, &VAO);

	unsigned int VBO;
	glGenBuffers(1, &VBO);

	unsigned int EBO;
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	return VAO;
}


void Simulation::loadShaders() {
	shader = Shader("shaders/vertex_shader.vert", "shaders/fragment_shader.frag");	// draws on picture texture
	backgroundShader = Shader("shaders/background_shader.vert", "shaders/background_shader.frag");	// background for the picture texture
	pictureShader = Shader("shaders/fluid/picture_shader.vert", "shaders/fluid/picture_shader.frag");	// computes new picture from vel field
	screenShader = Shader("shaders/window.vert", "shaders/window.frag");	// puts resulting picture on the actual window

	initialVField = Shader("shaders/fluid/initial_vfield.vert", "shaders/fluid/initial_vfield.frag");	// initial v field
	advectionShader = Shader("shaders/fluid/advection.vert", "shaders/fluid/advection.frag");		// advection for vel field
	diffusionShader = Shader("shaders/fluid/diffusion.vert", "shaders/fluid/diffusion.frag");		// viscous diffusion
	force_shader = Shader("shaders/fluid/force_shader.vert", "shaders/fluid/force_shader.frag");		// external forces
	pressureShader = Shader("shaders/fluid/pressure.vert", "shaders/fluid/pressure.frag");		// solves for pressure field
	projectionShader = Shader("shaders/fluid/projection.vert", "shaders/fluid/projection.frag");		// subtracts grad pressure field
	boundaryShader = Shader("shaders/fluid/boundary.vert", "shaders/fluid/boundary.frag");		// subtracts grad pressure field
}


void Simulation::loadFramebuffers() {
	createFramebuffer(pictureFramebuffer, pictureTexture);
	
	createFramebuffer(intermediatePictureFramebuffer, intermediatePictureTexture);
	
	createFramebuffer(velocityFramebuffer, velocityTexture);
	
	createFramebuffer(intermediateVelocityFramebuffer, intermediateVelocityTexture);
	
	createFramebuffer(pressureFramebuffer, pressureTexture);
	
	createFramebuffer(intermediatePressureFramebuffer, intermediatePressureTexture);
}


void Simulation::createFramebuffer(unsigned int& framebuffer, unsigned int& textureBuffer) {
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	glGenTextures(1, &textureBuffer);
	glBindTexture(GL_TEXTURE_2D, textureBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureBuffer, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
}


void Simulation::processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}


GLFWwindow* Simulation::createWindow() {
	GLFWwindow* window = glfwCreateWindow(width, height, "LearnOpenGL", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
	}
	return window;
}


void Simulation::initGLFW() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}


void Simulation::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	width = width;
	glViewport(0, 0, width, height);
}