//
// Created by Egor Orachyov on 2019-11-03.
//

#ifndef RENDERINGLIBRARY_VULKANCONTEXT_H
#define RENDERINGLIBRARY_VULKANCONTEXT_H

#include "VulkanDefinitions.h"
#include "VulkanUtils.h"
#include <vector>

/**
 * Handles vulkan instance setup. Defines physical
 * device and creates logical device for application.
 * defines queue families, finds graphics, present and transfer queues
 */
struct VulkanContext {

    /* Private section: setup vulkan instance */
    void createInstance();
    void destroyInstance();
    void fillRequiredExt(uint32 count, const char *const *ext);
    void pickPhysicalDevice();
    void createLogicalDevice();
    void destroyLogicalDevice();

    void checkSupportedExtensions();
    bool checkValidationLayers();

    void setupDebugMessenger();
    void destroyDebugMessenger();
    void outDeviceInfoVerbose();
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    void findQueueFamilies(VkPhysicalDevice device, QueueFamilyIndices &indices);

    VkResult createDebugUtilsMessengerEXT(
            const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
            const VkAllocationCallbacks *pAllocator,
            VkDebugUtilsMessengerEXT *pDebugMessenger
    );

    void destroyDebugUtilsMessengerEXT(
            VkDebugUtilsMessengerEXT debugMessenger,
            const VkAllocationCallbacks *pAllocator
    );

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData
    );

    VkFormatProperties getDeviceFormatProperties(VkFormat format) const;

    uint32_t getMemoryTypeIndex(uint32_t memoryTypeBits, VkFlags requirementsMask) const;

    void createBuffer(
            VkDeviceSize size,
            VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
            VkBuffer &outBuffer, VkDeviceMemory &outBufferMemory
    );

    void createBufferLocal(
            const void *data,
            VkDeviceSize size, VkBufferUsageFlags usage,
            VkBuffer &outBuffer, VkDeviceMemory &outBufferMemory
    );

    void copyBuffer(
            VkCommandPool commandPool,
            VkQueue queue,
            VkBuffer srcBuffer, VkBuffer dstBuffer,
            VkDeviceSize size
    );

    void updateBufferMemory(
            VkDeviceMemory bufferMemory,
            VkDeviceSize offset, VkDeviceSize size,
            const void *data
    );

    void createTextureImage(
            const void *imageData,
            uint32_t width, uint32_t height, uint32_t depth,
            VkImageType imageType, VkFormat format, VkImageTiling tiling,
            VkImage &outTextureImage, VkDeviceMemory &outTextureMemory,
            VkImageLayout textureLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    void createImage(
            uint32_t width, uint32_t height, uint32_t depth,
            VkImageType imageType, VkFormat format,
            VkImageTiling tiling, VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkImage &outImage, VkDeviceMemory &outImageMemory
    );

    void copyBufferToImage(
            VkBuffer buffer,
            VkImage image,
            uint32_t width, uint32_t height, uint32_t depth
    );

    void transitionImageLayout(
            VkImage image,
            VkImageLayout oldLayout,
            VkImageLayout newLayout
    );

    void createImageView(
            VkImageView &outImageView,
            VkImage image,
            VkImageViewType viewType, VkFormat format,
            const VkImageSubresourceRange &subResourceRange,
            VkComponentMapping components = {}
    );

    std::vector<const char *> requiredExtensions = {VK_KHR_SURFACE_EXTENSION_NAME};
    const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
    const bool enableValidationLayers = true;

    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    QueueFamilyIndices familyIndices = { };

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue transferQueue = VK_NULL_HANDLE;

    VkPhysicalDeviceProperties deviceProperties = {};
    VkPhysicalDeviceFeatures deviceFeatures = {};
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties = {};
    // TODO: init command pool
    VkCommandPool commandPool = VK_NULL_HANDLE;
};


#endif //RENDERINGLIBRARY_VULKANCONTEXT_H
