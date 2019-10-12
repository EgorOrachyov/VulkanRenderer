#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "ValidationLayers.h"
#include "UniformBuffer.h"

class VulkanTriangle;

struct SwapchainBuffer 
{
	VkImage					Image;
	VkImageView				View;
};

struct DepthBuffer
{
	VkImage					Image;
	VkImageView				View;
	VkDeviceMemory			Memory;
	VkFormat				Format;
};

// Scene that contains cube and a camera
class Scene
{
public:
	// Model view projection matrix
	float				MVP[16];
	// Uniform buffer for MVP matrix
	UniformBuffer		MVPUniform;

	void				Setup(const VulkanTriangle& t);
	void				Destroy();
};

class Utils
{
public:
	static bool GetMemoryType(const VkPhysicalDeviceMemoryProperties& deviceMemProperties, uint32_t memoryTypeBits, VkFlags requirementsMask, uint32_t& result);
};

class VulkanTriangle
{
private:
	int WindowWidth = 800;
	int WindowHeight = 600;

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	GLFWwindow						*window;

	VkInstance						vkInstance;
	
	ValidationLayers				validationLayers;
	
	VkSurfaceKHR					surface;
	VkFormat						surfaceFormat;

	VkSwapchainKHR					swapchain;
	uint32_t						swapchainImageCount;
	std::vector<SwapchainBuffer>	imageBuffers;

	VkPipelineLayout				pipelineLayout;

private:
	DepthBuffer								depthBuffer;

	std::vector<VkPhysicalDevice>			physicalDevices;

	int										choosedPhysDevice;
	VkPhysicalDeviceMemoryProperties		choosedDeviceMemProperties;
	VkPhysicalDeviceProperties				choosedDeviceProperties;
	
	uint32_t								queueFamilyCount;
	std::vector<VkQueueFamilyProperties>	queueFamilyProperties;

	uint32_t				graphicsQueueFamilyIndex;
	uint32_t				presentQueueFamilyIndex;

	VkDevice				device;

	VkSampleCountFlagBits SampleCount;

private:
	std::vector<const char*> GetRequiredInstanceExtensions();
	std::vector<const char*> GetRequiredDeviceExtensions();

	// Create GLFW window
	void CreateWindow();

	void InitVulkan();
	void CreateInstance();
	void CreateSurface();
	void SetupDebugMessenger();
	// Find physical devices, setup queue families, 
	// create logical device
	void EnumerateDevices();
	
	void CreateSwapchain();
	// Find queue family that supports graphics and present
	void FindQueueFamilyIndices();
	void FindSupportedFormats();
	void SetSurfaceCapabilities(VkSwapchainCreateInfoKHR &swapchainCreateInfo);
	void CreateSwapchainImages();

	void CreateDepthBuffer();

	void CreatePipelineLayout(uint32_t setLayoutCount, const VkDescriptorSetLayout* pSetLayouts);

	void MainLoop();

	void DestroyVulkan();
	void DestroyWindow();
	void DestroyInstance();
	void DestroyDevice();
	void DestroySwapchain();
	void DestroyDepthBuffer();

	void DestroyPipelineLayout();

public:
	void Start();

	const VkPhysicalDeviceMemoryProperties &GetChoosedDeviceMemProperties() const;
	const VkDevice GetDevice() const;
};