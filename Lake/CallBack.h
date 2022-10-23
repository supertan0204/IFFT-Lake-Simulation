#pragma once
#include <cstdio>
#include <GLFW/glfw3.h>
static void glfw_error_callback(int error, const char* description) {
	fprintf(stderr, "Error: %s\n", description);
	throw("glfw error");
}

static void glfw_window_close_callback(GLFWwindow* window) {
	printf("window closed\n");
}