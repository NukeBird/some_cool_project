#include <iostream>

#include <assimp/Importer.hpp>
#include <SOIL2/SOIL2.h>
#include <spdlog/spdlog.h>

#include "glfw_window.h"

using namespace std::literals;

int main()
{
	GlfwWindow window;
	window.
		setWindowLabel("Oh hi Mark"s).
		setWindowSize(800, 600);

	window.Run();

	std::cout << "Oh hi Mark" << std::endl;
	return EXIT_SUCCESS;
}