#include "framework.h"

// Store a global instance for use in the window proc call
DemoFramework* DemoFramework::instance = nullptr;

DemoFramework::DemoFramework(int argc, char* argv[], std::unique_ptr<Scene> _scene) : scene(std::move(_scene)) {
    instance = this;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--validate") == 0) {
            validate = true;
            continue;
        }
        if (strcmp(argv[i], "--width") == 0) {
            int32_t in_width = 0;
            if (i < argc - 1 && (in_width = std::stoi(argv[i + 1])) == 1) {
                if (in_width > 0) {
                    width = static_cast<uint32_t>(in_width);
                    i++;
                    continue;
                } else {
                    ERR_EXIT("The --width parameter must be greater than 0", "User Error");
                }
            }
            ERR_EXIT("The --width parameter must be followed by a number", "User Error");
        }
        if (strcmp(argv[i], "--height") == 0) {
            int32_t in_height = 0;
            if (i < argc - 1 && (in_height = std::stoi(argv[i + 1])) == 1) {
                if (in_height > 0) {
                    height = static_cast<uint32_t>(in_height);
                    i++;
                    continue;
                } else {
                    ERR_EXIT("The --height parameter must be greater than 0", "User Error");
                }
            }
            ERR_EXIT("The --height parameter must be followed by a number", "User Error");
        }
        if (strcmp(argv[i], "--force_errors") == 0) {
            force_errors = true;
            continue;
        }
        std::stringstream usage;
        usage << "Usage:\n  " << APP_SHORT_NAME
            << "\t[--width <width>] [--height <height>]: render window dimensions\n"
            << "\t[--validate] [--force_errors]: enable validation, and test error handling\n";

#if defined(_WIN32)
        MessageBoxA(nullptr, usage.str().c_str(), "Usage Error", MB_OK);
#else
        std::cerr << usage.str();
        std::cerr.flush();
#endif
        exit(1);
    }

    if (ENABLE_VULKAN_VALIDATION && !validate) {
        validate = true;
    }

    scene->init_vk(validate);

    create_window();

    scene->init_swapchain(window);
    scene->prepare(width, height, is_minimized, force_errors);
}

DemoFramework::~DemoFramework() {
    cleanup();
    instance = nullptr;
}

void DemoFramework::run() {
    // Set GLFW events callbacks
    glfwSetWindowSizeCallback(window, WindowSizeCallback);

    glfwShowWindow(window);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Time calculations
        // limit dt to maximum of 0.1 second, this is to mitigate large frame spikes when resizing window or Alt-TABing away
        const float max_dt = 1.0f / 10.0f;

	    static double previous_time = glfwGetTime();
        double current_time = glfwGetTime();
        float dt = min(static_cast<float>(current_time - previous_time), max_dt);
        previous_time = current_time;

        // Run this frame
        scene->frame(dt, width, height, is_minimized, force_errors);

        // FPS
        static float fps_timer = 0.0f;
        static int fps_counter = 0;
        fps_counter++;
        fps_timer += dt;
        if (fps_timer >= 1.0f) {
            fps_timer -= 1.0f;
            FPS = fps_counter;
            fps_counter = 0;
            std::string wt = APP_SHORT_NAME;
            wt += " (" + std::to_string(FPS);
            wt += ")";
            glfwSetWindowTitle(window, wt.c_str());
        }

        curFrame++;
    }
}

void DemoFramework::ResizeWindow(uint32_t new_width, uint32_t new_height)
{
    width = new_width;
    height = new_height;
    resize();
}

void DemoFramework::WindowSizeCallback(GLFWwindow* window, int width, int height)
{
    if (instance && width > 0 && height > 0 && (width != instance->width || height != instance->height))
    {
        instance->ResizeWindow(width, height);
    }
}

void DemoFramework::cleanup() {
    scene->cleanup(is_minimized);

    glfwDestroyWindow(window);
    glfwTerminate();

    scene->finalize();
}

void DemoFramework::create_window() {
    if (!glfwInit()) {
        ERR_EXIT("GLFW3 init failed", "GLFW3 Error");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, APP_SHORT_NAME, nullptr, nullptr);
    if (window == nullptr) {
        ERR_EXIT("GLFW3 window creation failed", "GLFW3 Error");
    }

    // Get window position and size
    int posX, posY;
    glfwGetWindowPos(window, &posX, &posY);

    int wwidth, wheight;
    glfwGetWindowSize(window, &wwidth, &wheight);

    // Handle multi-monitors and center window

    // Halve the window size and use it to adjust the window position to the center of the window
    wwidth >>= 1;
    wheight >>= 1;
    posX += wwidth;
    posY += wheight;

    // Get the list of monitors
    int count;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    if (monitors != nullptr) {
        // Figure out which monitor the window is in
        GLFWmonitor *owner = NULL;
        int owner_x, owner_y, owner_width, owner_height;

        for (int i = 0; i < count; i++)
        {
            // Get the monitor position
            int monitor_x, monitor_y;
            glfwGetMonitorPos(monitors[i], &monitor_x, &monitor_y);

            // Get the monitor size from its video mode
            int monitor_width, monitor_height;
            GLFWvidmode *monitor_vidmode = (GLFWvidmode*)glfwGetVideoMode(monitors[i]);

            if (monitor_vidmode == NULL)
                continue;
            
            monitor_width = monitor_vidmode->width;
            monitor_height = monitor_vidmode->height;

            // Set the owner to this monitor if the center of the window is within its bounding box
            if ((posX > monitor_x && posX < (monitor_x + monitor_width)) && (posY > monitor_y && posY < (monitor_y + monitor_height)))
            {
                owner = monitors[i];
                owner_x = monitor_x;
                owner_y = monitor_y;
                owner_width = monitor_width;
                owner_height = monitor_height;
            }
        }

        // Set the window position to the center of the owner monitor
        if (owner != nullptr) {
            glfwSetWindowPos(window, owner_x + (owner_width >> 1) - wwidth, owner_y + (owner_height >> 1) - wheight);
        }
    }
}

void DemoFramework::resize() {
    scene->resize(width, height, is_minimized, force_errors);		
}
