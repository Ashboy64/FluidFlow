#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <tgmath.h>
#include <direct.h>

#include "shader.h"

class Simulation {
public:
	// self explanatory
	float millisecondsPerFrame = 1000.0f/3.0f;

	// constructor
	Simulation();

	// runs the simulation
	void run();

private:

	int width;
	int height;

	GLFWwindow* window;

	unsigned int screenVAO;				// quad that covers whole screen
	unsigned int background;

	void loadShaders();					// load the shaders
	void loadFramebuffers();			// load the framebuffers and textures
	void createFramebuffer(unsigned int& framebuffer, unsigned int& textureBuffer);

	void drawInitialPicture();			// initial picture
	void drawInitialVelField();			// initial velocity field
	void drawInitialPressureField();	// initial pressure field

	// swaps two buffers (intermediate vel field and target vel field for instance)
	void swapBuffers(unsigned int sourceTexture, 
		unsigned int targetFramebuffer);

	// terms of Navier Stokes eqn
	void advection();
	void diffusion();
	void forceApplication();
	void pressureSolve();
	void projectToDivergenceFree();
	void boundaryConditions();

	void dyeApplication();

	// compute new image using current image and vel field
	void newImage();
	// draws pictureFramebuffer on actual screen
	void swapToMain();

	unsigned int sceneData();
	unsigned int sceneBackground();
	unsigned int windowQuad();
	unsigned int initialVelocityField();

	// openGl things
	void initGLFW();
	GLFWwindow* createWindow();
	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
	void processInput(GLFWwindow* window);


	// for computing forces from mouse movement
	float forceX;
	float forceY;

	float previousX;
	float previousY;

	// framebuffers and textures
	unsigned int pictureFramebuffer;
	unsigned int pictureTexture;

	unsigned int intermediatePictureFramebuffer;
	unsigned int intermediatePictureTexture;

	unsigned int velocityFramebuffer;
	unsigned int velocityTexture;

	unsigned int intermediateVelocityFramebuffer;
	unsigned int intermediateVelocityTexture;

	unsigned int pressureFramebuffer;
	unsigned int pressureTexture;

	unsigned int intermediatePressureFramebuffer;
	unsigned int intermediatePressureTexture;

	Shader shader;						// draws on picture texture
	Shader backgroundShader;			// background for the picture texture
	Shader pictureShader;				// computes new picture from vel field
	Shader screenShader;				// puts resulting picture on the actual window

	Shader initialVField;				// initial v field
	Shader advectionShader;				// advection for vel field
	Shader diffusionShader;				// viscous diffusion
	Shader force_shader;				// external forces
	Shader pressureShader;				// solves for pressure field
	Shader projectionShader;			// subtracts grad pressure field
	Shader boundaryShader;				// subtracts grad pressure field

};