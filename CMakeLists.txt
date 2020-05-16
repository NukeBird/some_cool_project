cmake_minimum_required(VERSION 3.0)
project(some_cool_project)

file(GLOB_RECURSE PROJECT_SOURCES ${CMAKE_SOURCE_DIR}/src/*)

add_executable(some_cool_project ${PROJECT_SOURCES})
