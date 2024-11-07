#pragma once

#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>

#include <sstream>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include <array>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include "config.h"

#ifndef NDEBUG
#define VERIFY(x) (assert(x))
#else
#define VERIFY(x) ((void)(x))
#endif

#ifdef _WIN32
#define ERR_EXIT(err_msg, err_class)                                          \
	do {                                                                      \
		MessageBoxA(nullptr, err_msg, err_class, MB_OK);                       \
		exit(1);                                                              \
	} while (0)
#else
#define ERR_EXIT(err_msg, err_class) \
	do {                             \
		printf("%s\n", err_msg);     \
		fflush(stdout);              \
		exit(1);                     \
	} while (0)
#endif

#include "VulkanWrapper.h"

#define GLFW_INCLUDE_NONE

#ifdef _WIN32
	#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
