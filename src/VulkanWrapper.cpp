#include "VulkanWrapper.h"
#include "common.h"

int validation_error = 0;

VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    std::ostringstream message;

    message << vk::to_string(vk::DebugUtilsMessageSeverityFlagBitsEXT(messageSeverity));
    message << " : " + vk::to_string(vk::DebugUtilsMessageTypeFlagsEXT(messageType));

    if (vk::DebugUtilsMessageTypeFlagsEXT(messageType) & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation) {
        validation_error = 1;
    }

    message << " - Message Id Number: " << std::to_string(pCallbackData->messageIdNumber);
    message << " | Message Id Name: " << (pCallbackData->pMessageIdName == nullptr ? "" : pCallbackData->pMessageIdName) << "\n\t"
            << pCallbackData->pMessage << "\n";

    if (pCallbackData->objectCount > 0) {
        message << "\n\tObjects - " << pCallbackData->objectCount << "\n";
        for (uint32_t object = 0; object < pCallbackData->objectCount; ++object) {
            message << "\t\tObject[" << object << "] - "
                    << vk::to_string(vk::ObjectType(pCallbackData->pObjects[object].objectType)) << ", Handle ";

            // Print handle correctly if it is a dispatchable handle - aka a pointer
            VkObjectType t = pCallbackData->pObjects[object].objectType;
            if (t == VK_OBJECT_TYPE_INSTANCE || t == VK_OBJECT_TYPE_PHYSICAL_DEVICE || t == VK_OBJECT_TYPE_DEVICE || t == VK_OBJECT_TYPE_COMMAND_BUFFER || t == VK_OBJECT_TYPE_QUEUE) {
                message << reinterpret_cast<void*>(static_cast<uintptr_t>(pCallbackData->pObjects[object].objectHandle));
            } else {
                message << pCallbackData->pObjects[object].objectHandle;
            }
            if (NULL != pCallbackData->pObjects[object].pObjectName && strlen(pCallbackData->pObjects[object].pObjectName) > 0) {
                message << ", Name \"" << pCallbackData->pObjects[object].pObjectName << "\"\n";
            } else {
                message << "\n";
            }
        }
    }
    if (pCallbackData->cmdBufLabelCount > 0) {
        message << "\n\tCommand Buffer Labels - " << pCallbackData->cmdBufLabelCount << "\n";
        for (uint32_t cmd_buf_label = 0; cmd_buf_label < pCallbackData->cmdBufLabelCount; ++cmd_buf_label) {
            message << "\t\tLabel[" << cmd_buf_label << "] - " << pCallbackData->pCmdBufLabels[cmd_buf_label].pLabelName << " { "
                    << pCallbackData->pCmdBufLabels[cmd_buf_label].color[0] << ", "
                    << pCallbackData->pCmdBufLabels[cmd_buf_label].color[1] << ", "
                    << pCallbackData->pCmdBufLabels[cmd_buf_label].color[2] << ", "
                    << pCallbackData->pCmdBufLabels[cmd_buf_label].color[2] << "}\n";
        }
    }

#ifdef _WIN32
    auto message_string = message.str();
    MessageBox(NULL, message_string.c_str(), "Alert", MB_OK);
#else
    std::cout << message.str() << std::endl; // use endl to force a flush
#endif
    return false; // Don't bail out, but keep going.
}


bool MemoryTypeFromProperties(uint32_t typeBits, vk::MemoryPropertyFlags RequirementsMask, uint32_t& typeIndex)
{
    auto MemoryProperties = GVulkanObjects.gpu.getMemoryProperties();

    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
        if ((typeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((MemoryProperties.memoryTypes[i].propertyFlags & RequirementsMask) == RequirementsMask) {
                typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }

    // No memory types matched, return failure
    return false;
}
