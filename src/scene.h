#pragma once

#include "common.h"
#include "TextureLoader.h"
#include "scene_data.h"
#include "MeshModel.h"

// Originally named: SwapchainImageResources, holds data required by frames-in-flight hence renamed to FrameResources
// The number of FrameResources is the number of Swapchain images.
// we do this to allow multiple frames to be processed and rendered to at the same time to minimize host idling as much
// as possible. We duplicate any resources that are accessed and modified during rendering a frame.
struct FrameResources {
	vk::Image image;
	vk::CommandBuffer cmd;
	vk::CommandBuffer graphics_to_present_cmd;
	vk::ImageView view;
	vk::Buffer uniform_buffer;
	vk::DeviceMemory uniform_memory;
	void *uniform_memory_ptr = nullptr;
	vk::Framebuffer framebuffer;
	vk::DescriptorSet descriptor_set;
};

struct DepthBuffer {
	vk::Format format;
	vk::Image image;
	vk::MemoryAllocateInfo mem_alloc;
	vk::DeviceMemory mem;
	vk::ImageView view;
};

class Scene {
public:
	Scene() {}
	virtual ~Scene() {}

	virtual void init_vk(bool validate = false);
    virtual void init_swapchain(GLFWwindow* whandle);
	virtual void prepare(uint32_t& width, uint32_t& height, bool &is_minimized, const bool &force_errors = false);
	virtual void resize(uint32_t &width, uint32_t &height, bool &is_minimized, const bool &force_errors);
	virtual void cleanup(const bool &is_minimized);
	virtual void finalize();
	virtual void frame(float dt, uint32_t& width, uint32_t& height, bool& is_minimized, const bool& force_errors);


protected:
	// Init and create actual scene objects (geometry, buffers, etc)
    virtual void init_scene() = 0;
	// Destroy scene objects (geometry, buffers, etc)
    virtual void cleanup_scene() = 0;

	// Setup graphics pipelines here
    virtual void create_graphics_pipelines() = 0;
	
	// Setup command buffer/draw commands here, called per swapchain frame
	// No need to call cmd.begin()/.end()
    virtual void populate_command_buffer(const vk::CommandBuffer& cmd, const FrameResources& frame, uint32_t width, uint32_t height) = 0;

	// This is called once to prepare the uniform data buffer for device mapping
	virtual std::pair<void*, size_t> create_uniform_data() = 0;
    virtual size_t get_uniform_buffer_size() = 0;

	// Called at start of a new frame for any preliminary code
	virtual void new_frame() {}
	// Main update function takes place before drawing, should update uniform buffer memory if anything is changing
	virtual void update(float dt, void* uniform_memory_ptr) = 0;
	
protected:
	bool is_prepared() const { return prepared; }
    void acquire_frame(uint32_t& width, uint32_t& height, bool& is_minimized, const bool& force_errors);
	void draw();
	void present(uint32_t &width, uint32_t &height, bool &is_minimized, const bool &force_errors);

protected:
	vk::Bool32 check_layers(const std::vector<const char *> &check_names, const std::vector<vk::LayerProperties> &layers);
	void create_surface();
	void create_device();
	vk::SurfaceFormatKHR pick_surface_format(const std::vector<vk::SurfaceFormatKHR> &surface_formats);
	void prepare_buffers(uint32_t& width, uint32_t& height, bool& is_minimized);
	void prepare_init_cmd();
	void prepare_depth(uint32_t width, uint32_t height, bool force_errors);
	bool memory_type_from_properties(uint32_t typeBits, vk::MemoryPropertyFlags requirements_mask, uint32_t &typeIndex);
	void prepare_textures();
    void prepare_texture_image(const char *filename, texture_object &tex_obj, vk::ImageTiling tiling, vk::ImageUsageFlags usage,
		vk::MemoryPropertyFlags required_props);
	void set_image_layout(vk::Image image, vk::ImageAspectFlags aspectMask, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
				vk::AccessFlags srcAccessMask, vk::PipelineStageFlags src_stages, vk::PipelineStageFlags dest_stages);
	void prepare_texture_buffer(const char *filename, texture_object &tex_obj);
	void prepare_uniform_data_buffers();
	void prepare_descriptor_layout();
	void prepare_render_pass();
	void prepare_pipeline();

	vk::ShaderModule prepare_vs();
	vk::ShaderModule prepare_fs();
	vk::ShaderModule prepare_shader_module(const uint32_t *code, size_t size);

	void prepare_descriptor_pool();
	void prepare_descriptor_set();

	void build_image_ownership_cmd(const FrameResources &frame);
	void prepare_framebuffers(uint32_t width, uint32_t height);
	void draw_build_cmd(const FrameResources &frame, uint32_t width, uint32_t height);
	void flush_init_cmd(const bool &force_errors);
	void destroy_texture(texture_object &tex_objs);
	void destroy_frame_resources();

	vk::PresentModeKHR 	presentMode = vk::PresentModeKHR::eFifo;
	int32_t 			gpu_number = -1;

	// Vulkan needs arrays of const char*
	std::vector<const char *> enabled_instance_extensions;
	std::vector<const char *> enabled_layers;
	std::vector<const char *> enabled_device_extensions;

	bool use_debug_messenger = false;
	bool invalid_gpu_selection = false;
	bool in_callback = false;
	bool prepared = false;

	vk::Instance 							inst;
	vk::DebugUtilsMessengerEXT 				debug_messenger;
	vk::PhysicalDeviceProperties 			gpu_props;
	std::vector<vk::QueueFamilyProperties> 	queue_props;
	vk::SurfaceKHR 							surface;
	vk::Queue 								graphics_queue;
	vk::Queue 								present_queue;
	vk::Format 								format;
	vk::ColorSpaceKHR 						color_space;
	std::array<vk::Fence, FRAME_LAG> 		fences;
	std::array<vk::Semaphore, FRAME_LAG> 	image_acquired_semaphores;
	std::array<vk::Semaphore, FRAME_LAG> 	draw_complete_semaphores;
	std::array<vk::Semaphore, FRAME_LAG> 	image_ownership_semaphores;
	vk::PhysicalDeviceMemoryProperties 		memory_properties;
	vk::SwapchainKHR 						swapchain;
	std::vector<FrameResources> 			frame_resources;


	vk::DescriptorPool		desc_pool;
	vk::DescriptorSet		desc_set;
	
	DepthBuffer				depth;

	uint32_t 	graphics_queue_family_index = 0;
	uint32_t 	present_queue_family_index = 0;
	uint32_t	frame_index = 0;
	bool 		separate_present_queue = false;
	uint32_t	current_buffer = 0;

	static int32_t const						texture_count = 1;
	texture_object								staging_texture;
	std::array<texture_object, texture_count>	textures;


protected:
	vk::CommandPool			cmd_pool;
	vk::CommandPool			present_cmd_pool;

	vk::CommandBuffer		cmd;  // Buffer for initialization commands
	vk::DescriptorSetLayout desc_layout;

    vk::Device			device;
    vk::PhysicalDevice	gpu;

    GLFWwindow*			window_handle = nullptr;

    vk::RenderPass		render_pass;
    vk::Pipeline		pipeline;
    vk::PipelineCache	pipelineCache;
    vk::PipelineLayout	pipeline_layout;

    bool				pause = false;
    float				aspect_ratio = 1.0f;
};
