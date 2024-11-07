#pragma once

#include "scene.h"

class DemoFramework {
public:
	DemoFramework(int argc, char* argv[], std::unique_ptr<Scene> _scene);
    virtual ~DemoFramework();

	void run();
    void ResizeWindow(uint32_t _width, uint32_t _height);
    
	static void WindowSizeCallback(GLFWwindow* window, int width, int height);

	void cleanup();
	
	void create_window();
	void resize();
	void draw();

private:
	std::unique_ptr<Scene>  scene;

	uint32_t    width = WINDOW_WIDTH;
	uint32_t    height = WINDOW_HEIGHT;
	bool 		quit = false;
	uint32_t	curFrame = 0;
	uint32_t	FPS = 0;
	bool        validate = false;
	bool        force_errors = false;
	bool 		is_minimized = false;

	GLFWwindow* window = 0;

	static DemoFramework*	instance;
};