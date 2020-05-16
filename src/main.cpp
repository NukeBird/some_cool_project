#define GLFW_INCLUDE_NONE
#include <iostream>

#include <globjects/globjects.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <SOIL2/SOIL2.h>
#include <spdlog/spdlog.h>
#include "model_importer.h"

#include "glfw_window.h"

using namespace std::literals;

int main()
{
	GlfwWindow window;
	window.
		setWindowLabel("Oh hi Mark"s).
		setWindowSize(800, 600);

	window.InitializeCallback = []
	{
		auto model = ModelImporter::load("data/matball.glb");

		spdlog::info("Model loaded: {0}", model != nullptr);
	};

	window.Run();



	return EXIT_SUCCESS;
}