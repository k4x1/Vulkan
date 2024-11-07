#pragma once

// The Vulkan defs here produce a more cautious implementation of Vulkan API
// Vulkan-Samples doesn't use these defs so they are not necesssary for practical applications
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_TYPESAFE_CONVERSION 1
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.hpp>
#include "volk.h"

extern int validation_error;

extern VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	void *pUserData);

// Contains vulkan objects that need to be globally accessible
struct VulkanObjects {
    vk::PhysicalDevice gpu;
    vk::Device device;
    vk::CommandBuffer cmd;
	bool initialized = false;
};
extern VulkanObjects GVulkanObjects;

// Utils
extern bool MemoryTypeFromProperties(uint32_t typeBits, vk::MemoryPropertyFlags RequirementsMask, uint32_t& typeIndex);
