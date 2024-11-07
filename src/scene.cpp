#include "scene.h"

#include "TextureLoader.h"
#include "ShaderLoader.h"

VulkanObjects GVulkanObjects;

void Scene::init_vk(bool validate)  {

	// See https://github.com/KhronosGroup/Vulkan-Hpp/pull/1755
	// Currently Vulkan-Hpp doesn't check for libvulkan.1.dylib
	// Which affects vkcube installation on Apple platforms.
	VkResult err = volkInitialize();
	if (err != VK_SUCCESS) {
		ERR_EXIT(
			"Unable to find the Vulkan runtime on the system.\n\n"
			"This likely indicates that no Vulkan capable drivers are installed.",
			"Installation Failure");
	}

	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	std::vector<char const *> instance_validation_layers = {"VK_LAYER_KHRONOS_validation"};

	// Look for validation layers
	vk::Bool32 validation_found = VK_FALSE;
	if (validate) {
		auto layers = vk::enumerateInstanceLayerProperties();
		VERIFY(layers.result == vk::Result::eSuccess);

		validation_found = check_layers(instance_validation_layers, layers.value);
		if (validation_found) {
			enabled_layers.push_back("VK_LAYER_KHRONOS_validation");
		}

		else {
			ERR_EXIT(
				"vkEnumerateInstanceLayerProperties failed to find required validation layer.\n\n"
				"Please look at the Getting Started guide for additional information.\n",
				"vkCreateInstance Failure");
		}
	}

	// Look for instance extensions
	vk::Bool32 surfaceExtFound = VK_FALSE;
	vk::Bool32 platformSurfaceExtFound = VK_FALSE;
	bool portabilityEnumerationActive = false;

	auto instance_extensions_return = vk::enumerateInstanceExtensionProperties();
	VERIFY(instance_extensions_return.result == vk::Result::eSuccess);

	for (const auto &extension : instance_extensions_return.value) {
		if (!strcmp(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, extension.extensionName)) {
			enabled_instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		} else if (!strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, extension.extensionName)) {
			use_debug_messenger = true;
			enabled_instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		} else if (!strcmp(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME, extension.extensionName)) {
			// We want cube to be able to enumerate drivers that support the portability_subset extension, so we have to enable the
			// portability enumeration extension.
			portabilityEnumerationActive = true;
			enabled_instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
		} else if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, extension.extensionName)) {
			surfaceExtFound = 1;
			enabled_instance_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
		}
#if defined(VK_USE_PLATFORM_WIN32_KHR)
		else if (!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, extension.extensionName)) {
			platformSurfaceExtFound = 1;
			enabled_instance_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
		}
#elif defined(VK_USE_PLATFORM_XCB_KHR)
		else if (!strcmp(VK_KHR_XCB_SURFACE_EXTENSION_NAME, extension.extensionName)) {
			platformSurfaceExtFound = 1;
			enabled_instance_extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
		}
#endif
	}

	if (!surfaceExtFound) {
		ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_SURFACE_EXTENSION_NAME
				" extension.\n\n"
				"Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
				"Please look at the Getting Started guide for additional information.\n",
				"vkCreateInstance Failure");
	}

	if (!platformSurfaceExtFound) {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
		ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_WIN32_SURFACE_EXTENSION_NAME
				" extension.\n\n"
				"Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
				"Please look at the Getting Started guide for additional information.\n",
				"vkCreateInstance Failure");
#elif defined(VK_USE_PLATFORM_XCB_KHR)
		ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_XCB_SURFACE_EXTENSION_NAME
				" extension.\n\n"
				"Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
				"Please look at the Getting Started guide for additional information.\n",
				"vkCreateInstance Failure");
#endif
	}

	vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
														vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
	vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
													vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
													vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
	auto debug_utils_create_info = vk::DebugUtilsMessengerCreateInfoEXT({}, severityFlags, messageTypeFlags,
																		&debug_messenger_callback, static_cast<void *>(&in_callback));

	auto const app = vk::ApplicationInfo()
						.setPApplicationName(APP_SHORT_NAME)
						.setApplicationVersion(0)
						.setPEngineName(APP_SHORT_NAME)
						.setEngineVersion(0)
						.setApiVersion(VK_API_VERSION_1_0);
	auto const inst_info = vk::InstanceCreateInfo()
							.setFlags(portabilityEnumerationActive ? vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR
																	: static_cast<vk::InstanceCreateFlagBits>(0))
							.setPNext((use_debug_messenger && validate) ? &debug_utils_create_info : nullptr)
							.setPApplicationInfo(&app)
							.setPEnabledLayerNames(enabled_layers)
							.setPEnabledExtensionNames(enabled_instance_extensions);

	auto instance_result = vk::createInstance(inst_info);
	if (instance_result.result == vk::Result::eErrorIncompatibleDriver) {
		ERR_EXIT(
			"Cannot find a compatible Vulkan installable client driver (ICD).\n\n"
			"Please look at the Getting Started guide for additional information.\n",
			"vkCreateInstance Failure");
	} else if (instance_result.result == vk::Result::eErrorExtensionNotPresent) {
		ERR_EXIT(
			"Cannot find a specified extension library.\n"
			"Make sure your layers path is set appropriately.\n",
			"vkCreateInstance Failure");
	} else if (instance_result.result != vk::Result::eSuccess) {
		ERR_EXIT(
			"vkCreateInstance failed.\n\n"
			"Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
			"Please look at the Getting Started guide for additional information.\n",
			"vkCreateInstance Failure");
	}
	inst = instance_result.value;
	VULKAN_HPP_DEFAULT_DISPATCHER.init(inst);

	if (use_debug_messenger) {
		auto create_debug_messenger_return = inst.createDebugUtilsMessengerEXT(debug_utils_create_info);
		VERIFY(create_debug_messenger_return.result == vk::Result::eSuccess);
		debug_messenger = create_debug_messenger_return.value;
	}

	auto physical_device_return = inst.enumeratePhysicalDevices();
	VERIFY(physical_device_return.result == vk::Result::eSuccess);
	auto physical_devices = physical_device_return.value;

	if (physical_devices.size() <= 0) {
		ERR_EXIT(
			"vkEnumeratePhysicalDevices reported zero accessible devices.\n\n"
			"Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
			"Please look at the Getting Started guide for additional information.\n",
			"vkEnumeratePhysicalDevices Failure");
	}

	if (invalid_gpu_selection || (gpu_number >= 0 && !(static_cast<uint32_t>(gpu_number) < physical_devices.size()))) {
		fprintf(stderr, "GPU %d specified is not present, GPU count = %zu\n", gpu_number, physical_devices.size());
		ERR_EXIT("Specified GPU number is not present", "User Error");
	}
	// Try to auto select most suitable device
	if (gpu_number == -1) {
		constexpr uint32_t device_type_count = static_cast<uint32_t>(vk::PhysicalDeviceType::eCpu) + 1;
		std::array<uint32_t, device_type_count> count_device_type{};

		for (uint32_t i = 0; i < physical_devices.size(); i++) {
			const auto physicalDeviceProperties = physical_devices[i].getProperties();
			assert(physicalDeviceProperties.deviceType <= vk::PhysicalDeviceType::eCpu);
			count_device_type[static_cast<int>(physicalDeviceProperties.deviceType)]++;
		}

		std::array<vk::PhysicalDeviceType, device_type_count> const device_type_preference = {
			vk::PhysicalDeviceType::eDiscreteGpu, vk::PhysicalDeviceType::eIntegratedGpu, vk::PhysicalDeviceType::eVirtualGpu,
			vk::PhysicalDeviceType::eCpu, vk::PhysicalDeviceType::eOther};

		vk::PhysicalDeviceType search_for_device_type = vk::PhysicalDeviceType::eDiscreteGpu;
		for (uint32_t i = 0; i < sizeof(device_type_preference) / sizeof(vk::PhysicalDeviceType); i++) {
			if (count_device_type[static_cast<int>(device_type_preference[i])]) {
				search_for_device_type = device_type_preference[i];
				break;
			}
		}

		for (uint32_t i = 0; i < physical_devices.size(); i++) {
			const auto physicalDeviceProperties = physical_devices[i].getProperties();
			if (physicalDeviceProperties.deviceType == search_for_device_type) {
				gpu_number = i;
				break;
			}
		}
	}
	assert(gpu_number >= 0);
	gpu = physical_devices[gpu_number];
	{
		auto physicalDeviceProperties = gpu.getProperties();
		fprintf(stderr, "Selected GPU %d: %s, type: %s\n", gpu_number, physicalDeviceProperties.deviceName.data(),
				to_string(physicalDeviceProperties.deviceType).c_str());
	}

	// Look for device extensions
	vk::Bool32 swapchainExtFound = VK_FALSE;

	auto device_extension_return = gpu.enumerateDeviceExtensionProperties();
	VERIFY(device_extension_return.result == vk::Result::eSuccess);

	for (const auto &extension : device_extension_return.value) {
		if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, extension.extensionName)) {
			swapchainExtFound = 1;
			enabled_device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		} else if (!strcmp("VK_KHR_portability_subset", extension.extensionName)) {
			enabled_device_extensions.push_back("VK_KHR_portability_subset");
		}
	}

	if (!swapchainExtFound) {
		ERR_EXIT("vkEnumerateDeviceExtensionProperties failed to find the " VK_KHR_SWAPCHAIN_EXTENSION_NAME
				" extension.\n\n"
				"Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
				"Please look at the Getting Started guide for additional information.\n",
				"vkCreateInstance Failure");
	}

	gpu.getProperties(&gpu_props);

	// Call with nullptr data to get count
	queue_props = gpu.getQueueFamilyProperties();
	assert(queue_props.size() >= 1);

	// Query fine-grained feature support for this device.
	//  If app has specific feature requirements it should check supported
	//  features based on this query
	vk::PhysicalDeviceFeatures physDevFeatures;
	gpu.getFeatures(&physDevFeatures);
}

void Scene::init_swapchain(GLFWwindow* whandle)
{
	window_handle = whandle;

	create_surface();

	// Iterate over each queue to learn whether it supports presenting:
	std::vector<vk::Bool32> supportsPresent;
	for (uint32_t i = 0; i < static_cast<uint32_t>(queue_props.size()); i++) {
		auto supports = gpu.getSurfaceSupportKHR(i, surface);
		VERIFY(supports.result == vk::Result::eSuccess);
		supportsPresent.push_back(supports.value);
	}

	uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
	uint32_t presentQueueFamilyIndex = UINT32_MAX;
	for (uint32_t i = 0; i < static_cast<uint32_t>(queue_props.size()); i++) {
		if (queue_props[i].queueFlags & vk::QueueFlagBits::eGraphics) {
			if (graphicsQueueFamilyIndex == UINT32_MAX) {
				graphicsQueueFamilyIndex = i;
			}

			if (supportsPresent[i] == VK_TRUE) {
				graphicsQueueFamilyIndex = i;
				presentQueueFamilyIndex = i;
				break;
			}
		}
	}

	if (presentQueueFamilyIndex == UINT32_MAX) {
		// If didn't find a queue that supports both graphics and present,
		// then
		// find a separate present queue.
		for (uint32_t i = 0; i < queue_props.size(); ++i) {
			if (supportsPresent[i] == VK_TRUE) {
				presentQueueFamilyIndex = i;
				break;
			}
		}
	}

	// Generate error if could not find both a graphics and a present queue
	if (graphicsQueueFamilyIndex == UINT32_MAX || presentQueueFamilyIndex == UINT32_MAX) {
		ERR_EXIT("Could not find both graphics and present queues\n", "Swapchain Initialization Failure");
	}

	graphics_queue_family_index = graphicsQueueFamilyIndex;
	present_queue_family_index = presentQueueFamilyIndex;
	separate_present_queue = (graphics_queue_family_index != present_queue_family_index);

	create_device();

	graphics_queue = device.getQueue(graphics_queue_family_index, 0);
	if (!separate_present_queue) {
		present_queue = graphics_queue;
	} else {
		present_queue = device.getQueue(present_queue_family_index, 0);
	}

	// Get the list of VkFormat's that are supported:
	auto surface_formats_return = gpu.getSurfaceFormatsKHR(surface);
	VERIFY(surface_formats_return.result == vk::Result::eSuccess);

	vk::SurfaceFormatKHR surfaceFormat = pick_surface_format(surface_formats_return.value);
	format = surfaceFormat.format;
	color_space = surfaceFormat.colorSpace;

	// Create semaphores to synchronize acquiring presentable buffers before
	// rendering and waiting for drawing to be complete before presenting
	auto const semaphoreCreateInfo = vk::SemaphoreCreateInfo();

	// Create fences that we can use to throttle if we get too far
	// ahead of the image presents
	auto const fence_ci = vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);
	for (uint32_t i = 0; i < FRAME_LAG; i++) {
		vk::Result result = device.createFence(&fence_ci, nullptr, &fences[i]);
		VERIFY(result == vk::Result::eSuccess);

		result = device.createSemaphore(&semaphoreCreateInfo, nullptr, &image_acquired_semaphores[i]);
		VERIFY(result == vk::Result::eSuccess);

		result = device.createSemaphore(&semaphoreCreateInfo, nullptr, &draw_complete_semaphores[i]);
		VERIFY(result == vk::Result::eSuccess);

		if (separate_present_queue) {
			result = device.createSemaphore(&semaphoreCreateInfo, nullptr, &image_ownership_semaphores[i]);
			VERIFY(result == vk::Result::eSuccess);
		}
	}

	// Get Memory information and properties
	memory_properties = gpu.getMemoryProperties();
}

void Scene::prepare(uint32_t& width, uint32_t& height, bool& is_minimized, const bool& force_errors)
{
    aspect_ratio = static_cast<float>(width) / static_cast<float>(height);

	prepare_buffers(width, height, is_minimized);
	if (is_minimized) {
		prepared = false;
		return;
	}

    prepare_init_cmd();

    GVulkanObjects.gpu = gpu;
    GVulkanObjects.device = device;
    GVulkanObjects.cmd = cmd;
    GVulkanObjects.initialized = true;

	prepare_depth(width, height, force_errors);

	prepare_textures();
	prepare_uniform_data_buffers();

	prepare_descriptor_layout();
	prepare_render_pass();
	prepare_pipeline();

	for (auto &frame : frame_resources) {
		auto alloc_return = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo()
															.setCommandPool(cmd_pool)
															.setLevel(vk::CommandBufferLevel::ePrimary)
															.setCommandBufferCount(1));
		VERIFY(alloc_return.result == vk::Result::eSuccess);
		frame.cmd = alloc_return.value[0];
	}

	if (separate_present_queue) {
		auto present_cmd_pool_return =
			device.createCommandPool(vk::CommandPoolCreateInfo().setQueueFamilyIndex(present_queue_family_index));
		VERIFY(present_cmd_pool_return.result == vk::Result::eSuccess);
		present_cmd_pool = present_cmd_pool_return.value;

		for (auto &frame : frame_resources) {
			auto alloc_cmd_return = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo()
																	.setCommandPool(present_cmd_pool)
																	.setLevel(vk::CommandBufferLevel::ePrimary)
																	.setCommandBufferCount(1));
			VERIFY(alloc_cmd_return.result == vk::Result::eSuccess);
			frame.graphics_to_present_cmd = alloc_cmd_return.value[0];
			build_image_ownership_cmd(frame);
		}
	}

	prepare_descriptor_pool();
	prepare_descriptor_set();

	prepare_framebuffers(width, height);

    init_scene();

	for (const auto &frame : frame_resources) {
		draw_build_cmd(frame, width, height);
	}

	// Prepare functions above may generate pipeline commands
	// that need to be flushed before beginning the render loop
	flush_init_cmd(force_errors);
	if (staging_texture.buffer) {
		destroy_texture(staging_texture);
	}

	current_buffer = 0;
	prepared = true;
}

void Scene::resize(uint32_t &width, uint32_t &height, bool &is_minimized, const bool &force_errors)
{
	// Don't react to resize until after first initialization.
	if (!prepared) {
		if (is_minimized) {
			prepare(width, height, is_minimized, force_errors);
		}
		return;
	}

	// In order to properly resize the window, we must re-create the swapchain
	// AND redo the command buffers, etc.
	//
	// First, perform part of the cleanup() function:
	prepared = false;
	auto result = device.waitIdle();
	VERIFY(result == vk::Result::eSuccess);
    destroy_frame_resources();

	// Second, re-perform the prepare() function, which will re-create the swapchain.
	prepare(width, height, is_minimized, force_errors);
}

void Scene::acquire_frame(uint32_t &width, uint32_t &height, bool &is_minimized, const bool &force_errors) {
	// Ensure no more than FRAME_LAG renderings are outstanding
	const vk::Result wait_result = device.waitForFences(fences[frame_index], VK_TRUE, UINT64_MAX);
	VERIFY(wait_result == vk::Result::eSuccess || wait_result == vk::Result::eTimeout);
	device.resetFences({fences[frame_index]});

	vk::Result acquire_result;
	do {
		acquire_result =
			device.acquireNextImageKHR(swapchain, UINT64_MAX, image_acquired_semaphores[frame_index], vk::Fence(), &current_buffer);
		if (acquire_result == vk::Result::eErrorOutOfDateKHR) {
			// demo.swapchain is out of date (e.g. the window was resized) and
			// must be recreated:
			resize(width, height, is_minimized, force_errors);
		} else if (acquire_result == vk::Result::eSuboptimalKHR) {
			// swapchain is not as optimal as it could be, but the platform's
			// presentation engine will still present the image correctly.
			break;
		} else if (acquire_result == vk::Result::eErrorSurfaceLostKHR) {
			inst.destroySurfaceKHR(surface);
			create_surface();
			resize(width, height, is_minimized, force_errors);
		} else {
			VERIFY(acquire_result == vk::Result::eSuccess);
		}
	} while (acquire_result != vk::Result::eSuccess);
}

void Scene::draw() {
    // Wait for the image acquired semaphore to be signaled to ensure
    // that the image won't be rendered to until the presentation
    // engine has fully released ownership to the application, and it is
    // okay to render to the image.
    vk::PipelineStageFlags const pipe_stage_flags = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    auto submit_result = graphics_queue.submit(vk::SubmitInfo()
                                                    .setWaitDstStageMask(pipe_stage_flags)
                                                    .setWaitSemaphores(image_acquired_semaphores[frame_index])
                                                    .setCommandBuffers(frame_resources[current_buffer].cmd)
                                                    .setSignalSemaphores(draw_complete_semaphores[frame_index]),
        fences[frame_index]);
    VERIFY(submit_result == vk::Result::eSuccess);

    if (separate_present_queue) {
            // If we are using separate queues, change image ownership to the
            // present queue before presenting, waiting for the draw complete
            // semaphore and signalling the ownership released semaphore when
            // finished
            auto change_owner_result = present_queue.submit(vk::SubmitInfo()
                                                                .setWaitDstStageMask(pipe_stage_flags)
                                                                .setWaitSemaphores(draw_complete_semaphores[frame_index])
                                                                .setCommandBuffers(frame_resources[current_buffer].graphics_to_present_cmd)
                                                                .setSignalSemaphores(image_ownership_semaphores[frame_index]));
            VERIFY(change_owner_result == vk::Result::eSuccess);
    }
}

void Scene::present(uint32_t &width, uint32_t &height, bool &is_minimized, const bool &force_errors) {
	const auto presentInfo = vk::PresentInfoKHR()
								.setWaitSemaphores(separate_present_queue ? image_ownership_semaphores[frame_index]
																		: draw_complete_semaphores[frame_index])
								.setSwapchains(swapchain)
								.setImageIndices(current_buffer);

	// If we are using separate queues we have to wait for image ownership,
	// otherwise wait for draw complete
	auto present_result = present_queue.presentKHR(&presentInfo);
	frame_index += 1;
	frame_index %= FRAME_LAG;
	if (present_result == vk::Result::eErrorOutOfDateKHR) {
		// swapchain is out of date (e.g. the window was resized) and
		// must be recreated:
		resize(width, height, is_minimized, force_errors);
	} else if (present_result == vk::Result::eSuboptimalKHR) {
		// SUBOPTIMAL could be due to resize
		vk::SurfaceCapabilitiesKHR surfCapabilities;
		auto caps_result = gpu.getSurfaceCapabilitiesKHR(surface, &surfCapabilities);
		VERIFY(caps_result == vk::Result::eSuccess);
		if (surfCapabilities.currentExtent.width != width || surfCapabilities.currentExtent.height != height) {
			resize(width, height, is_minimized, force_errors);
		}
	} else if (present_result == vk::Result::eErrorSurfaceLostKHR) {
		inst.destroySurfaceKHR(surface);
		create_surface();
		resize(width, height, is_minimized, force_errors);
	} else {
		VERIFY(present_result == vk::Result::eSuccess);
	}
}

void Scene::cleanup(const bool &is_minimized) {
	prepared = false;
	auto result = device.waitIdle();
	VERIFY(result == vk::Result::eSuccess);

	cleanup_scene();

	if (!is_minimized) {
		destroy_frame_resources();
	}

	// Wait for fences from present operations
	for (uint32_t i = 0; i < FRAME_LAG; i++) {
		device.destroyFence(fences[i]);
		device.destroySemaphore(image_acquired_semaphores[i]);
		device.destroySemaphore(draw_complete_semaphores[i]);
		if (separate_present_queue) {
			device.destroySemaphore(image_ownership_semaphores[i]);
		}
	}

	device.destroySwapchainKHR(swapchain);

	// Clear vulkan objects
	GVulkanObjects = VulkanObjects{};

	device.destroy();
	inst.destroySurfaceKHR(surface);
}

void Scene::finalize() {
	if (use_debug_messenger) {
		inst.destroyDebugUtilsMessengerEXT(debug_messenger);
	}
	inst.destroy();
}


void Scene::frame(float dt, uint32_t& width, uint32_t& height, bool& is_minimized, const bool& force_errors) {
	// If we're puased, pass scene delta time = 0.0f
	float scene_dt = pause? 0.0f : dt;

    new_frame();

	if (is_prepared()) {
			acquire_frame(width, height, is_minimized, force_errors);
			update(scene_dt, frame_resources[current_buffer].uniform_memory_ptr);
			draw();
			present(width, height, is_minimized, force_errors);
    }
}


vk::Bool32 Scene::check_layers(const std::vector<const char *> &check_names, const std::vector<vk::LayerProperties> &layers) {
	for (const auto &name : check_names) {
		vk::Bool32 found = VK_FALSE;
		for (const auto &layer : layers) {
			if (!strcmp(name, layer.layerName)) {
				found = VK_TRUE;
				break;
			}
		}
		if (!found) {
			fprintf(stderr, "Cannot find layer: %s\n", name);
			return 0;
		}
	}
	return VK_TRUE;
}

void Scene::create_surface() {
	// Create a WSI surface for the window:
	// GLFW3 is a C library so can only use the Vulkan C API
	VkSurfaceKHR c_surface = {};
	VkResult result = glfwCreateWindowSurface(inst, window_handle, nullptr, &c_surface);
	VERIFY(result == VK_SUCCESS);
		
	// Convert VkSurfaceKHR to the C++ handle: vk::SurfaceKHR
	surface = vk::SurfaceKHR(c_surface);
}

void Scene::create_device() {
	float priorities = 0.0;

	std::vector<vk::DeviceQueueCreateInfo> queues;
	queues.push_back(vk::DeviceQueueCreateInfo().setQueueFamilyIndex(graphics_queue_family_index).setQueuePriorities(priorities));

	if (separate_present_queue) {
		queues.push_back(
			vk::DeviceQueueCreateInfo().setQueueFamilyIndex(present_queue_family_index).setQueuePriorities(priorities));
	}

	auto deviceInfo = vk::DeviceCreateInfo().setQueueCreateInfos(queues).setPEnabledExtensionNames(enabled_device_extensions);
	auto device_return = gpu.createDevice(deviceInfo);
	VERIFY(device_return.result == vk::Result::eSuccess);
	device = device_return.value;
	VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
}

vk::SurfaceFormatKHR Scene::pick_surface_format(const std::vector<vk::SurfaceFormatKHR> &surface_formats) {
	// Prefer non-SRGB formats...
	for (const auto &surface_format : surface_formats) {
		const vk::Format format = surface_format.format;

		if (format == vk::Format::eR8G8B8A8Unorm || format == vk::Format::eB8G8R8A8Unorm ||
			format == vk::Format::eA2B10G10R10UnormPack32 || format == vk::Format::eA2R10G10B10UnormPack32 ||
			format == vk::Format::eA1R5G5B5UnormPack16 || format == vk::Format::eR5G6B5UnormPack16 ||
			format == vk::Format::eR16G16B16A16Sfloat) {
			return surface_format;
		}
	}

	printf("Can't find our preferred formats... Falling back to first exposed format. Rendering may be incorrect.\n");

	assert(surface_formats.size() >= 1);
	return surface_formats[0];
}

void Scene::prepare_buffers(uint32_t& width, uint32_t& height, bool& is_minimized) {
	vk::SwapchainKHR oldSwapchain = swapchain;

	// Check the surface capabilities and formats
	auto surface_capabilities_return = gpu.getSurfaceCapabilitiesKHR(surface);
	VERIFY(surface_capabilities_return.result == vk::Result::eSuccess);
	auto surfCapabilities = surface_capabilities_return.value;

	auto present_modes_return = gpu.getSurfacePresentModesKHR(surface);
	VERIFY(present_modes_return.result == vk::Result::eSuccess);
	auto present_modes = present_modes_return.value;

	vk::Extent2D swapchainExtent;
	// width and height are either both -1, or both not -1.
	if (surfCapabilities.currentExtent.width == static_cast<uint32_t>(-1)) {
		// If the surface size is undefined, the size is set to
		// the size of the images requested.
		swapchainExtent.width = width;
		swapchainExtent.height = height;
	} else {
		// If the surface size is defined, the swap chain size must match
		swapchainExtent = surfCapabilities.currentExtent;
		width = surfCapabilities.currentExtent.width;
		height = surfCapabilities.currentExtent.height;
	}

	if (width == 0 || height == 0) {
		is_minimized = true;
		return;
	} else {
		is_minimized = false;
	}
	// The FIFO present mode is guaranteed by the spec to be supported
	// and to have no tearing.  It's a great default present mode to use.
	vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;

	//  There are times when you may wish to use another present mode.  The
	//  following code shows how to select them, and the comments provide some
	//  reasons you may wish to use them.
	//
	// It should be noted that Vulkan 1.0 doesn't provide a method for
	// synchronizing rendering with the presentation engine's display.  There
	// is a method provided for throttling rendering with the display, but
	// there are some presentation engines for which this method will not work.
	// If an application doesn't throttle its rendering, and if it renders much
	// faster than the refresh rate of the display, this can waste power on
	// mobile devices.  That is because power is being spent rendering images
	// that may never be seen.

	// VK_PRESENT_MODE_IMMEDIATE_KHR is for applications that don't care about
	// tearing, or have some way of synchronizing their rendering with the
	// display.
	// VK_PRESENT_MODE_MAILBOX_KHR may be useful for applications that
	// generally render a new presentable image every refresh cycle, but are
	// occasionally early.  In this case, the application wants the new image
	// to be displayed instead of the previously-queued-for-presentation image
	// that has not yet been displayed.
	// VK_PRESENT_MODE_FIFO_RELAXED_KHR is for applications that generally
	// render a new presentable image every refresh cycle, but are occasionally
	// late.  In this case (perhaps because of stuttering/latency concerns),
	// the application wants the late image to be immediately displayed, even
	// though that may mean some tearing.

	if (presentMode != swapchainPresentMode) {
		for (const auto &mode : present_modes) {
			if (mode == presentMode) {
				swapchainPresentMode = mode;
				break;
			}
		}
	}

	if (swapchainPresentMode != presentMode) {
		ERR_EXIT("Present mode specified is not supported\n", "Present mode unsupported");
	}

	// Determine the number of VkImages to use in the swap chain.
	// Application desires to acquire 3 images at a time for triple
	// buffering
	uint32_t desiredNumOfSwapchainImages = 3;
	if (desiredNumOfSwapchainImages < surfCapabilities.minImageCount) {
		desiredNumOfSwapchainImages = surfCapabilities.minImageCount;
	}

	// If maxImageCount is 0, we can ask for as many images as we want,
	// otherwise
	// we're limited to maxImageCount
	if ((surfCapabilities.maxImageCount > 0) && (desiredNumOfSwapchainImages > surfCapabilities.maxImageCount)) {
		// Application must settle for fewer images than desired:
		desiredNumOfSwapchainImages = surfCapabilities.maxImageCount;
	}

	vk::SurfaceTransformFlagBitsKHR preTransform;
	if (surfCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) {
		preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
	} else {
		preTransform = surfCapabilities.currentTransform;
	}

	// Find a supported composite alpha mode - one of these is guaranteed to be set
	vk::CompositeAlphaFlagBitsKHR compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	std::array<vk::CompositeAlphaFlagBitsKHR, 4> compositeAlphaFlags = {
		vk::CompositeAlphaFlagBitsKHR::eOpaque,
		vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
		vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
		vk::CompositeAlphaFlagBitsKHR::eInherit,
	};
	for (const auto &compositeAlphaFlag : compositeAlphaFlags) {
		if (surfCapabilities.supportedCompositeAlpha & compositeAlphaFlag) {
			compositeAlpha = compositeAlphaFlag;
			break;
		}
	}

	auto swapchain_return = device.createSwapchainKHR(vk::SwapchainCreateInfoKHR()
														.setSurface(surface)
														.setMinImageCount(desiredNumOfSwapchainImages)
														.setImageFormat(format)
														.setImageColorSpace(color_space)
														.setImageExtent({swapchainExtent.width, swapchainExtent.height})
														.setImageArrayLayers(1)
														.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
														.setImageSharingMode(vk::SharingMode::eExclusive)
														.setPreTransform(preTransform)
														.setCompositeAlpha(compositeAlpha)
														.setPresentMode(swapchainPresentMode)
														.setClipped(true)
														.setOldSwapchain(oldSwapchain));
	VERIFY(swapchain_return.result == vk::Result::eSuccess);
	swapchain = swapchain_return.value;

	// If we just re-created an existing swapchain, we should destroy the
	// old
	// swapchain at this point.
	// Note: destroying the swapchain also cleans up all its associated
	// presentable images once the platform is done with them.
	if (oldSwapchain) {
		device.destroySwapchainKHR(oldSwapchain);
	}

	auto swapchain_images_return = device.getSwapchainImagesKHR(swapchain);
	VERIFY(swapchain_images_return.result == vk::Result::eSuccess);
	frame_resources.resize(swapchain_images_return.value.size());

	for (uint32_t i = 0; i < frame_resources.size(); ++i) {
		auto color_image_view = vk::ImageViewCreateInfo()
									.setViewType(vk::ImageViewType::e2D)
									.setFormat(format)
									.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

		frame_resources[i].image = swapchain_images_return.value[i];

		color_image_view.image = frame_resources[i].image;

		auto image_view_return = device.createImageView(color_image_view);
		VERIFY(image_view_return.result == vk::Result::eSuccess);
		frame_resources[i].view = image_view_return.value;
	}

}

void Scene::prepare_init_cmd() {
	auto cmd_pool_return = device.createCommandPool(vk::CommandPoolCreateInfo().setQueueFamilyIndex(graphics_queue_family_index));
	VERIFY(cmd_pool_return.result == vk::Result::eSuccess);
	cmd_pool = cmd_pool_return.value;

	auto cmd_return = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo()
														.setCommandPool(cmd_pool)
														.setLevel(vk::CommandBufferLevel::ePrimary)
														.setCommandBufferCount(1));
	VERIFY(cmd_return.result == vk::Result::eSuccess);
	cmd = cmd_return.value[0];

	auto result = cmd.begin(vk::CommandBufferBeginInfo());
	VERIFY(result == vk::Result::eSuccess);
}

void Scene::prepare_depth(uint32_t width, uint32_t height, bool force_errors) {
	depth.format = vk::Format::eD16Unorm;

	auto const image = vk::ImageCreateInfo()
							.setImageType(vk::ImageType::e2D)
							.setFormat(depth.format)
							.setExtent({width, height, 1})
							.setMipLevels(1)
							.setArrayLayers(1)
							.setSamples(vk::SampleCountFlagBits::e1)
							.setTiling(vk::ImageTiling::eOptimal)
							.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
							.setSharingMode(vk::SharingMode::eExclusive)
							.setInitialLayout(vk::ImageLayout::eUndefined);

	auto result = device.createImage(&image, nullptr, &depth.image);
	VERIFY(result == vk::Result::eSuccess);

	vk::MemoryRequirements mem_reqs;
	device.getImageMemoryRequirements(depth.image, &mem_reqs);

	depth.mem_alloc.setAllocationSize(mem_reqs.size);
	depth.mem_alloc.setMemoryTypeIndex(0);

	auto const pass = MemoryTypeFromProperties(mem_reqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal,
													depth.mem_alloc.memoryTypeIndex);
	VERIFY(pass);

	result = device.allocateMemory(&depth.mem_alloc, nullptr, &depth.mem);
	VERIFY(result == vk::Result::eSuccess);

	result = device.bindImageMemory(depth.image, depth.mem, 0);
	VERIFY(result == vk::Result::eSuccess);

	auto view = vk::ImageViewCreateInfo()
					.setImage(depth.image)
					.setViewType(vk::ImageViewType::e2D)
					.setFormat(depth.format)
					.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1));
	if (force_errors) {
		// Intentionally force a bad pNext value to generate a validation layer error
		view.pNext = &image;
	}
	result = device.createImageView(&view, nullptr, &depth.view);
	VERIFY(result == vk::Result::eSuccess);
}

void Scene::prepare_textures() {		
	vk::Format const tex_format = vk::Format::eR8G8B8A8Unorm;
	vk::FormatProperties props;
	gpu.getFormatProperties(tex_format, &props);

	// Two possible types of texture creation are handled:
	// 1. Linear tiling texture: store texture in linear memory buffer in row-major order (desktops)
	// 2. Optimal tiling texture: store texture in a platform-specific memory optimized way (may not be stored in a linear memory buffer, for example on handheld devices)
	// Vulkan specs: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageTiling.html

	// This example uses one texture only: texture_count = 1, so we are setting textures[0] directly here

	if (props.linearTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage) {
		// Device can texture using linear textures
        TextureLoader::CreateTexture2D(tex_files[0], textures[0]);

		// Nothing in the pipeline needs to be complete to start, and don't allow fragment
		// shader to run until layout transition completes
		TextureLoader::SetImageLayout(textures[0].image, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::ePreinitialized, textures[0].imageLayout, vk::AccessFlagBits(), vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eFragmentShader);

		// staging_texture is not used when we create a linear texture
		staging_texture.image = vk::Image();
	} else if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage) {
		// Must use staging buffer to copy linear texture to optimized. Following code:
		// 1. Creates a staging linear memory buffer
		// 2. Loads texture file into staging memory
		// 3. Creates optimized memory buffer
		// 4. Transfer texture data to optimized layout memory

		TextureLoader::CreateBuffer2D(tex_files[0], staging_texture);
		TextureLoader::CreateOptimalTexture2DFromBuffer(staging_texture, textures[0]);

		// staging_texture is used so we need to flush the pipeline commands queue later (before we use texture)
	} else {
		assert(!"No support for R8G8B8A8_UNORM as texture image format");
	}

	auto const samplerInfo = vk::SamplerCreateInfo()
								.setMagFilter(vk::Filter::eNearest)
								.setMinFilter(vk::Filter::eNearest)
								.setMipmapMode(vk::SamplerMipmapMode::eNearest)
								.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
								.setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
								.setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
								.setMipLodBias(0.0f)
								.setAnisotropyEnable(VK_FALSE)
								.setMaxAnisotropy(1)
								.setCompareEnable(VK_FALSE)
								.setCompareOp(vk::CompareOp::eNever)
								.setMinLod(0.0f)
								.setMaxLod(0.0f)
								.setBorderColor(vk::BorderColor::eFloatOpaqueWhite)
								.setUnnormalizedCoordinates(VK_FALSE);

	auto result = device.createSampler(&samplerInfo, nullptr, &textures[0].sampler);
	VERIFY(result == vk::Result::eSuccess);

	auto const viewInfo = vk::ImageViewCreateInfo()
							.setImage(textures[0].image)
							.setViewType(vk::ImageViewType::e2D)
							.setFormat(tex_format)
							.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

	result = device.createImageView(&viewInfo, nullptr, &textures[0].view);
	VERIFY(result == vk::Result::eSuccess);
}

void Scene::prepare_uniform_data_buffers() {
	auto [data, data_size] = create_uniform_data();

	auto const buf_info = vk::BufferCreateInfo().setSize(data_size).setUsage(vk::BufferUsageFlagBits::eUniformBuffer);

	for (auto &frame : frame_resources) {
		auto result = device.createBuffer(&buf_info, nullptr, &frame.uniform_buffer);
		VERIFY(result == vk::Result::eSuccess);

		vk::MemoryRequirements mem_reqs;
		device.getBufferMemoryRequirements(frame.uniform_buffer, &mem_reqs);

		auto mem_alloc = vk::MemoryAllocateInfo().setAllocationSize(mem_reqs.size).setMemoryTypeIndex(0);

		bool const pass = MemoryTypeFromProperties(
			mem_reqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			mem_alloc.memoryTypeIndex);
		VERIFY(pass);

		result = device.allocateMemory(&mem_alloc, nullptr, &frame.uniform_memory);
		VERIFY(result == vk::Result::eSuccess);

		result = device.mapMemory(frame.uniform_memory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags(),
								&frame.uniform_memory_ptr);
		VERIFY(result == vk::Result::eSuccess);

		memcpy(frame.uniform_memory_ptr, data, data_size);

		result = device.bindBufferMemory(frame.uniform_buffer, frame.uniform_memory, 0);
		VERIFY(result == vk::Result::eSuccess);
	}
}

void Scene::prepare_descriptor_layout() {
	std::array<vk::DescriptorSetLayoutBinding, 2> const layout_bindings = {
		vk::DescriptorSetLayoutBinding()
			.setBinding(0)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(1)
			.setStageFlags(vk::ShaderStageFlagBits::eVertex)
			.setPImmutableSamplers(nullptr),
		vk::DescriptorSetLayoutBinding()
			.setBinding(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setDescriptorCount(texture_count)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment)
			.setPImmutableSamplers(nullptr)};

	auto const descriptor_layout = vk::DescriptorSetLayoutCreateInfo().setBindings(layout_bindings);

	auto result = device.createDescriptorSetLayout(&descriptor_layout, nullptr, &desc_layout);
	VERIFY(result == vk::Result::eSuccess);

	auto const pPipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo().setSetLayouts(desc_layout);

	result = device.createPipelineLayout(&pPipelineLayoutCreateInfo, nullptr, &pipeline_layout);
	VERIFY(result == vk::Result::eSuccess);
}

void Scene::prepare_render_pass() {
	// The initial layout for the color and depth attachments will be LAYOUT_UNDEFINED
	// because at the start of the renderpass, we don't care about their contents.
	// At the start of the subpass, the color attachment's layout will be transitioned
	// to LAYOUT_COLOR_ATTACHMENT_OPTIMAL and the depth stencil attachment's layout
	// will be transitioned to LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL.  At the end of
	// the renderpass, the color attachment's layout will be transitioned to
	// LAYOUT_PRESENT_SRC_KHR to be ready to present.  This is all done as part of
	// the renderpass, no barriers are necessary.
	std::array<vk::AttachmentDescription, 2> const attachments = {
		vk::AttachmentDescription()
			.setFormat(format)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eStore)
			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::ePresentSrcKHR),
		vk::AttachmentDescription()
			.setFormat(depth.format)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)};

	auto const color_reference = vk::AttachmentReference().setAttachment(0).setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	auto const depth_reference =
		vk::AttachmentReference().setAttachment(1).setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	auto const subpass = vk::SubpassDescription()
							.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
							.setColorAttachments(color_reference)
							.setPDepthStencilAttachment(&depth_reference);

	vk::PipelineStageFlags stages = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
	std::array<vk::SubpassDependency, 2> const dependencies = {
		vk::SubpassDependency()  // Depth buffer is shared between swapchain images
			.setSrcSubpass(VK_SUBPASS_EXTERNAL)
			.setDstSubpass(0)
			.setSrcStageMask(stages)
			.setDstStageMask(stages)
			.setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
			.setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite)
			.setDependencyFlags(vk::DependencyFlags()),
		vk::SubpassDependency()  // Image layout transition
			.setSrcSubpass(VK_SUBPASS_EXTERNAL)
			.setDstSubpass(0)
			.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setSrcAccessMask(vk::AccessFlagBits())
			.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead)
			.setDependencyFlags(vk::DependencyFlags()),
	};

	const auto render_pass_result = device.createRenderPass(
		vk::RenderPassCreateInfo().setAttachments(attachments).setSubpasses(subpass).setDependencies(dependencies));
	VERIFY(render_pass_result.result == vk::Result::eSuccess);
	render_pass = render_pass_result.value;
}

void Scene::prepare_pipeline() {
	create_graphics_pipelines();
}

void Scene::prepare_descriptor_pool() {
	std::array<vk::DescriptorPoolSize, 2> const poolSizes = {
		vk::DescriptorPoolSize()
			.setType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(static_cast<uint32_t>(frame_resources.size())),
		vk::DescriptorPoolSize()
			.setType(vk::DescriptorType::eCombinedImageSampler)
			.setDescriptorCount(static_cast<uint32_t>(frame_resources.size()) * texture_count)};

	auto const descriptor_pool =
		vk::DescriptorPoolCreateInfo().setMaxSets(static_cast<uint32_t>(frame_resources.size())).setPoolSizes(poolSizes);

	auto result = device.createDescriptorPool(&descriptor_pool, nullptr, &desc_pool);
	VERIFY(result == vk::Result::eSuccess);
}

void Scene::prepare_descriptor_set() {
	auto const alloc_info = vk::DescriptorSetAllocateInfo().setDescriptorPool(desc_pool).setSetLayouts(desc_layout);

	auto buffer_info = vk::DescriptorBufferInfo().setOffset(0).setRange(get_uniform_buffer_size());

	std::array<vk::DescriptorImageInfo, texture_count> tex_descs;
	for (uint32_t i = 0; i < texture_count; i++) {
		tex_descs[i].setSampler(textures[i].sampler);
		tex_descs[i].setImageView(textures[i].view);
		tex_descs[i].setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	}

	std::array<vk::WriteDescriptorSet, 2> writes;
	writes[0].setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eUniformBuffer).setPBufferInfo(&buffer_info);
	writes[1]
		.setDstBinding(1)
		.setDescriptorCount(texture_count)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setImageInfo(tex_descs);

	for (auto &frame : frame_resources) {
		auto result = device.allocateDescriptorSets(&alloc_info, &frame.descriptor_set);
		VERIFY(result == vk::Result::eSuccess);

		buffer_info.setBuffer(frame.uniform_buffer);
		writes[0].setDstSet(frame.descriptor_set);
		writes[1].setDstSet(frame.descriptor_set);
		device.updateDescriptorSets(writes, {});
	}
}

void Scene::build_image_ownership_cmd(const FrameResources &frame) {
	auto result = frame.graphics_to_present_cmd.begin(
		vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse));
	VERIFY(result == vk::Result::eSuccess);

	auto const image_ownership_barrier =
		vk::ImageMemoryBarrier()
			.setSrcAccessMask(vk::AccessFlags())
			.setDstAccessMask(vk::AccessFlags())
			.setOldLayout(vk::ImageLayout::ePresentSrcKHR)
			.setNewLayout(vk::ImageLayout::ePresentSrcKHR)
			.setSrcQueueFamilyIndex(graphics_queue_family_index)
			.setDstQueueFamilyIndex(present_queue_family_index)
			.setImage(frame.image)
			.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

	frame.graphics_to_present_cmd.pipelineBarrier(vk::PipelineStageFlagBits::eBottomOfPipe,
																	vk::PipelineStageFlagBits::eBottomOfPipe,
																	vk::DependencyFlagBits(), {}, {}, image_ownership_barrier);

	result = frame.graphics_to_present_cmd.end();
	VERIFY(result == vk::Result::eSuccess);
}

void Scene::prepare_framebuffers(uint32_t width, uint32_t height) {
	std::array<vk::ImageView, 2> attachments;
	attachments[1] = depth.view;

	for (auto &frame : frame_resources) {
		attachments[0] = frame.view;
		auto const framebuffer_return = device.createFramebuffer(vk::FramebufferCreateInfo()
																	.setRenderPass(render_pass)
																	.setAttachments(attachments)
																	.setWidth(width)
																	.setHeight(height)
																	.setLayers(1));
		VERIFY(framebuffer_return.result == vk::Result::eSuccess);
		frame.framebuffer = framebuffer_return.value;
	}
}

void Scene::draw_build_cmd(const FrameResources &frame, uint32_t width, uint32_t height) {
	const auto commandBuffer = frame.cmd;

	auto result = commandBuffer.begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse));
	VERIFY(result == vk::Result::eSuccess);

	populate_command_buffer(commandBuffer, frame, width, height);

	if (separate_present_queue) {
		// We have to transfer ownership from the graphics queue family to the
		// present queue family to be able to present.  Note that we don't have
		// to transfer from present queue family back to graphics queue family at
		// the start of the next frame because we don't care about the image's
		// contents at that point.
		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eBottomOfPipe, vk::DependencyFlagBits(), {}, {},
			vk::ImageMemoryBarrier()
				.setSrcAccessMask(vk::AccessFlags())
				.setDstAccessMask(vk::AccessFlags())
				.setOldLayout(vk::ImageLayout::ePresentSrcKHR)
				.setNewLayout(vk::ImageLayout::ePresentSrcKHR)
				.setSrcQueueFamilyIndex(graphics_queue_family_index)
				.setDstQueueFamilyIndex(present_queue_family_index)
				.setImage(frame.image)
				.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));
	}

	result = commandBuffer.end();
	VERIFY(result == vk::Result::eSuccess);
}

void Scene::flush_init_cmd(const bool &force_errors) {
	auto result = cmd.end();
	VERIFY(result == vk::Result::eSuccess);

	auto fenceInfo = vk::FenceCreateInfo();
	if (force_errors) {
		// Remove sType to intentionally force validation layer errors.
		fenceInfo.sType = vk::StructureType::eRenderPassBeginInfo;
	}
	auto fence_return = device.createFence(fenceInfo);
	VERIFY(fence_return.result == vk::Result::eSuccess);
	auto fence = fence_return.value;

	result = graphics_queue.submit(vk::SubmitInfo().setCommandBuffers(cmd), fence);
	VERIFY(result == vk::Result::eSuccess);

	result = device.waitForFences(fence, VK_TRUE, UINT64_MAX);
	VERIFY(result == vk::Result::eSuccess);

	device.freeCommandBuffers(cmd_pool, cmd);
	device.destroyFence(fence);
}

void Scene::destroy_texture(texture_object &tex_objs) {
	// clean up staging resources
	device.freeMemory(tex_objs.mem);
	if (tex_objs.image) {
		device.destroyImage(tex_objs.image);
	}
	if (tex_objs.buffer) {
		device.destroyBuffer(tex_objs.buffer);
	}
}

void Scene::destroy_frame_resources() {
	device.destroyDescriptorPool(desc_pool);

	device.destroyPipeline(pipeline);
	device.destroyPipelineCache(pipelineCache);
	device.destroyRenderPass(render_pass);
	device.destroyPipelineLayout(pipeline_layout);
	device.destroyDescriptorSetLayout(desc_layout);

	for (const auto &tex : textures) {
		device.destroyImageView(tex.view);
		device.destroyImage(tex.image);
		device.freeMemory(tex.mem);
		device.destroySampler(tex.sampler);
	}

	device.destroyImageView(depth.view);
	device.destroyImage(depth.image);
	device.freeMemory(depth.mem);

	for (const auto &resource : frame_resources) {
		device.destroyFramebuffer(resource.framebuffer);
		device.destroyImageView(resource.view);
		device.freeCommandBuffers(cmd_pool, {resource.cmd});
		device.destroyBuffer(resource.uniform_buffer);
		device.unmapMemory(resource.uniform_memory);
		device.freeMemory(resource.uniform_memory);
	}

	device.destroyCommandPool(cmd_pool);
	if (separate_present_queue) {
		device.destroyCommandPool(present_cmd_pool);
	}
}
