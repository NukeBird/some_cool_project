#define GLFW_INCLUDE_NONE
#include <iostream>
#include "model_importer.h"

#include <GLFW/glfw3.h>

int main()
{
	std::cout << "Oh hi Mark" << std::endl;

	glfwInit();
	auto w = glfwCreateWindow(1, 1, "", 0, 0);
	glfwMakeContextCurrent(w);

	globjects::init([](const char* name) 
	{
		return glfwGetProcAddress(name);
	});

	ModelImporter::load("blah.glb");
	return EXIT_SUCCESS;
}