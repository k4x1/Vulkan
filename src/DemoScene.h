#pragma once

#include "scene.h"

struct UBO_Textured {
    glm::mat4 model;
    glm::mat4 viewproj;
};


// Input structure
// key that isn't down or released has a value of 0
// when key is pressed, its value is incremented every frame.
// when key is released, its value is set back to -1 for that frame
struct SceneControls {
    int left = 0;
    int right = 0;
    int space = 0;

    // called in main loop to reset state, clears released state (-1) from last frame
    void on_new_frame()
    {
        if (left == -1) {
            left = 0;
        }
        if (right == -1) {
            right = 0;
        }
        if (space == -1) {
            space = 0;
        }
    }
};

class DemoScene : public Scene {
protected:
    virtual void init_scene() override;
    virtual void cleanup_scene() override;

    virtual void create_graphics_pipelines() override;
    
    // Called N times. each frame in-flight has its own command buffer that needs to be populated
    virtual void populate_command_buffer(const vk::CommandBuffer& cmd, const FrameResources& frame, uint32_t width, uint32_t height) override;

    virtual std::pair<void*, size_t> create_uniform_data() override;
    virtual size_t get_uniform_buffer_size() override { return sizeof UBO_Textured; }

    virtual void new_frame() override;
    virtual void update(float dt, void* uniform_memory_ptr) override;

private:
    // Staging uniform data, will be copied to device-mapped memory ptr to update uniform data
    UBO_Textured uniform_data;
    
    vk::Buffer          vertex_buffer;
    vk::DeviceMemory    vertex_buffer_memory;

private:
    SceneControls controls;

    float spin_speed = 0.0f;
    float spin_control = 0.0f;

    std::unique_ptr<MeshModel> m_model;

    // Camera
    glm::vec3 eye { 0.0f, 3.0f, 5.0f };
    glm::vec3 origin { 0, 0, 0 };
    glm::vec3 up { 0.0f, 1.0f, 0.0 };

    // MVP
    glm::mat4 model_matrix = {};
    glm::mat4 view_matrix = {};
    glm::mat4 projection_matrix = {};
};
