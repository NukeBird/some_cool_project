#define GLFW_INCLUDE_NONE
#include <iostream>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include "model_importer.h"

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

	auto model = ModelImporter::load("blah.glb");

	spdlog::info("Model loaded: {0}", model != nullptr);

	return EXIT_SUCCESS;
}