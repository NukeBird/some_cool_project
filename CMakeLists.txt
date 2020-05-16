cmake_minimum_required(VERSION 3.0)
project(some_cool_project)

file(GLOB_RECURSE PROJECT_SOURCES ${CMAKE_SOURCE_DIR}/src/*)

find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(globjects CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(soil2 CONFIG REQUIRED)

add_executable(some_cool_project ${PROJECT_SOURCES})
target_link_libraries(some_cool_project glm assimp::assimp globjects::globjects
	glfw spdlog::spdlog spdlog::spdlog_header_only soil2)
