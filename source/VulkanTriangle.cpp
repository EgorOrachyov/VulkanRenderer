#include "VulkanTriangle.h"
#include "Shader.h"
#include "VertexBuffer.h"
#include <stdexcept>
#include <iostream>
#include <cassert>

void VulkanTriangle::Start()
{
	// use 1 sample
	SampleCount = VK_SAMPLE_COUNT_1_BIT;

	CreateWindow();
	InitVulkan();

	// create shader
	Shader cubeShader;
	// create descriptor set layout for this shader
	cubeShader.Init(device);

	// load spirv binary to shader
	cubeShader.Load("shaders/vert.spv", "shaders/frag.spv");

	// create scene
	Scene scene;
	scene.Setup(*this);

	// init uniform buffer
	// NOTE: if not working, put binding after Unmap()
	cubeShader.mvpUniform.Init(choosedDeviceMemProperties, device, sizeof(scene.MVP));
	cubeShader.mvpUniform.MapAndCopy(scene.MVP, sizeof(scene.MVP));
	cubeShader.mvpUniform.Unmap();

	VertexBuffer vertexBuffer;
	// NOTE: if not working, put binding after Unmap()
	vertexBuffer.Init(choosedDeviceMemProperties, device);
	vertexBuffer.MapAndCopy();
	vertexBuffer.Unmap();


	// get descriptor set layouts of all shaders,
	// and pool sizes for creating descriptor pool
	const int setLayoutCount = 1;
	const int poolSizeCount = 1;
	VkDescriptorPoolSize poolSizes[] = { cubeShader.GetPoolSize() };
	VkDescriptorSetLayout setLayouts[] = { cubeShader.GetDescriptorSetLayout() };

	// create pipeline layout
	CreatePipelineLayout(setLayouts, setLayoutCount);
	// and descriptor pool
	CreateDescriptorPool(poolSizes, poolSizeCount);
	
	// allocate descriptor sets using pool
	AllocateDescriptorSets(setLayouts, setLayoutCount);

	// write content of descriptor set;
	// here only uniform buffer will be written
	VkWriteDescriptorSet writes[] = { cubeShader.GetWriteDescriptorSet(descriptorSets[0]) };
	vkUpdateDescriptorSets(device, 1, writes, 0, NULL);

	CreateRenderPass();
	CreateFramebuffers();



	MainLoop();



	scene.Destroy();

	vertexBuffer.Destroy();
	cubeShader.mvpUniform.Destroy();
	cubeShader.Destroy();


	DestroyFramebuffers();
	DestroyRenderPass();

	DestroyPipelineLayout();
	DestroyDescriptorPool();

	DestroyVulkan();
	DestroyWindow();
}

const VkPhysicalDeviceMemoryProperties &VulkanTriangle::GetChoosedDeviceMemProperties() const
{
	return choosedDeviceMemProperties;
}

const VkDevice VulkanTriangle::GetDevice() const
{
	return device;
}

void VulkanTriangle::CreateWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(WindowWidth, WindowHeight, "Vulkan Triangle", nullptr, nullptr);
}

void VulkanTriangle::MainLoop()
{
	while (!glfwWindowShouldClose(window)) 
	{
		glfwPollEvents();
	}
}

void VulkanTriangle::InitVulkan()
{
	CreateInstance();
	SetupDebugMessenger();
	CreateSurface();
	EnumerateDevices();
	CreateSwapchain();
	CreateDepthBuffer();
}

void VulkanTriangle::CreateInstance()
{
	if (enableValidationLayers)
	{
		validationLayers.Check();
	}

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// set extensions for vulkan instance
	auto extensions = GetRequiredInstanceExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;

	// if debug
	if (enableValidationLayers) 
	{
		validationLayers.SetValidationLayersForInstance(createInfo, debugCreateInfo);
	}
	else 
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	VkResult r = vkCreateInstance(&createInfo, TR_VK_ALLOCATION_CALLBACKS_MARK, &vkInstance);
	assert(r == VK_SUCCESS);
}

void VulkanTriangle::CreateSurface()
{
	VkResult r = glfwCreateWindowSurface(vkInstance, window, TR_VK_ALLOCATION_CALLBACKS_MARK, &surface);
	assert(r == VK_SUCCESS);
}

void VulkanTriangle::SetupDebugMessenger()
{
	if (enableValidationLayers)
	{
		validationLayers.SetupDebugMessenger(vkInstance);
	}
}

void VulkanTriangle::EnumerateDevices()
{
	uint32_t physicalDeviceCount = 0;

	// get physical device count
	vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, NULL);
	assert(physicalDeviceCount > 0);

	physicalDevices.resize(physicalDeviceCount);

	// get physical devices
	VkResult r = vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, physicalDevices.data());
	assert(r == VK_SUCCESS && physicalDeviceCount >= 1);


	// for testing, choose first device
	choosedPhysDevice = 0;

	// get choosed device properties
	vkGetPhysicalDeviceMemoryProperties(physicalDevices[choosedPhysDevice], &choosedDeviceMemProperties);
	vkGetPhysicalDeviceProperties(physicalDevices[choosedPhysDevice], &choosedDeviceProperties);
	// device extension properties?

	// get queue family count 
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[choosedPhysDevice], &queueFamilyCount, NULL);
	assert(queueFamilyCount >= 1);

	queueFamilyProperties.resize(queueFamilyCount);

	// get queue families
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[choosedPhysDevice], &queueFamilyCount, queueFamilyProperties.data());
	assert(queueFamilyCount >= 1);

	VkDeviceQueueCreateInfo queueInfo = {};

	bool found = false;
	for (unsigned int i = 0; i < queueFamilyCount; i++) 
	{
		// find family that supports graphics
		if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			queueInfo.queueFamilyIndex = i;
			found = true;

			break;
		}
	}

	assert(found);

	float queuePriorities[1] = { 0.0f };

	queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfo.pNext = NULL;
	queueInfo.queueCount = 1;
	queueInfo.pQueuePriorities = queuePriorities;

	auto deviceExtensions = GetRequiredDeviceExtensions();

	// init logical device
	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pNext = NULL;
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.pQueueCreateInfos = &queueInfo;
	deviceInfo.pEnabledFeatures = NULL;

	// set extensions for this device
	deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers)
	{
		validationLayers.SetValidationLayersForDevice(deviceInfo);
	}
	else
	{
		deviceInfo.enabledLayerCount = 0;
		deviceInfo.ppEnabledLayerNames = NULL;
	}

	r = vkCreateDevice(physicalDevices[choosedPhysDevice], &deviceInfo, TR_VK_ALLOCATION_CALLBACKS_MARK, &device);
	assert(r == VK_SUCCESS);
}

void VulkanTriangle::CreateSwapchain()
{
	// find necessary properties
	FindQueueFamilyIndices();
	FindSupportedFormats();

	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.pNext = NULL;
	swapchainCreateInfo.surface = surface;
	
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
	swapchainCreateInfo.clipped = true;
	swapchainCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	// set surface capabilities
	SetSurfaceCapabilities(swapchainCreateInfo);

	uint32_t queueFamilyIndices[2] = { (uint32_t)graphicsQueueFamilyIndex, (uint32_t)presentQueueFamilyIndex };

	// check queues, if they are from same queue families
	if (graphicsQueueFamilyIndex == presentQueueFamilyIndex)
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = NULL;
	}
	else
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}

 	VkResult r = vkCreateSwapchainKHR(device, &swapchainCreateInfo, TR_VK_ALLOCATION_CALLBACKS_MARK, &swapchain);
	assert(r == VK_SUCCESS);

	CreateSwapchainImages();
}

void VulkanTriangle::CreateSwapchainImages()
{
	VkResult r;

	// get count of swapchain images
	r = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, NULL);
	assert(r == VK_SUCCESS);

	VkImage* swapchainImages = (VkImage*)malloc(swapchainImageCount * sizeof(VkImage));

	// get these images
	r = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages);
	assert(r == VK_SUCCESS);

	// set images to current class
	imageBuffers.resize(swapchainImageCount);

	for (uint32_t i = 0; i < swapchainImageCount; i++)
	{
		imageBuffers[i].Image = swapchainImages[i];
	}

	free(swapchainImages);

	// create image view for each swapchain image
	for (uint32_t i = 0; i < swapchainImageCount; i++)
	{
		VkImageViewCreateInfo colorImageView = {};
		colorImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		colorImageView.pNext = NULL;
		colorImageView.flags = 0;

		// set image for this view
		colorImageView.image = imageBuffers[i].Image;
		colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		
		colorImageView.format = surfaceFormat;
		colorImageView.components.r = VK_COMPONENT_SWIZZLE_R;
		colorImageView.components.g = VK_COMPONENT_SWIZZLE_G;
		colorImageView.components.b = VK_COMPONENT_SWIZZLE_B;
		colorImageView.components.a = VK_COMPONENT_SWIZZLE_A;
		
		colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorImageView.subresourceRange.baseMipLevel = 0;
		colorImageView.subresourceRange.levelCount = 1;
		colorImageView.subresourceRange.baseArrayLayer = 0;
		colorImageView.subresourceRange.layerCount = 1;

		r = vkCreateImageView(device, &colorImageView, TR_VK_ALLOCATION_CALLBACKS_MARK, &imageBuffers[i].View);
		assert(r == VK_SUCCESS);
	}
}

void VulkanTriangle::CreateDepthBuffer()
{
	VkFormat depthFormat = depthBuffer.Format = VK_FORMAT_D16_UNORM;

	VkFormatProperties properties;
	vkGetPhysicalDeviceFormatProperties(physicalDevices[choosedPhysDevice], depthFormat, &properties);

	VkImageCreateInfo depthImageInfo  = {};

	if (properties.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		depthImageInfo.tiling = VK_IMAGE_TILING_LINEAR;
	}
	else if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	}
	else
	{
		assert(false && "VK_FORMAT_D16_UNORM unsupported");
	}

	depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	depthImageInfo.pNext = NULL;
	depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
	depthImageInfo.format = depthFormat;
	// set size of the window
	depthImageInfo.extent.width = WindowWidth;
	depthImageInfo.extent.height = WindowHeight;
	depthImageInfo.extent.depth = 1;
	depthImageInfo.mipLevels = 1;
	depthImageInfo.arrayLayers = 1;
	depthImageInfo.samples = SampleCount;
	depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// this image will be used for depth
	depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depthImageInfo.queueFamilyIndexCount = 0;
	depthImageInfo.pQueueFamilyIndices = NULL;
	depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	depthImageInfo.flags = 0;

	VkResult r = vkCreateImage(device, &depthImageInfo, TR_VK_ALLOCATION_CALLBACKS_MARK, &depthBuffer.Image);
	assert(r == VK_SUCCESS);

	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(device, depthBuffer.Image, &memReqs);

	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.pNext = NULL;
	memAllocInfo.allocationSize = memReqs.size;

	// VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT:
	//   this memory must be accessed efficiently by device
	uint32_t memoryTypeIndex;
	bool foundMemoryType = Utils::GetMemoryType(choosedDeviceMemProperties, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryTypeIndex);
	assert(foundMemoryType);
		
	memAllocInfo.memoryTypeIndex = memoryTypeIndex;

	r = vkAllocateMemory(device, &memAllocInfo, TR_VK_ALLOCATION_CALLBACKS_MARK, &depthBuffer.Memory);
	assert(r == VK_SUCCESS);

	r = vkBindImageMemory(device, depthBuffer.Image, depthBuffer.Memory, 0);
	assert(r == VK_SUCCESS);

	VkImageViewCreateInfo depthViewInfo = {};
	depthViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthViewInfo.pNext = NULL;
	depthViewInfo.image = VK_NULL_HANDLE;
	depthViewInfo.format = depthFormat;
	depthViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	depthViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	depthViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	depthViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
	// depth image view
	depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthViewInfo.subresourceRange.baseMipLevel = 0;
	depthViewInfo.subresourceRange.levelCount = 1;
	depthViewInfo.subresourceRange.baseArrayLayer = 0;
	depthViewInfo.subresourceRange.layerCount = 1;
	depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthViewInfo.flags = 0;
	depthViewInfo.image = depthBuffer.Image;

	r = vkCreateImageView(device, &depthViewInfo, TR_VK_ALLOCATION_CALLBACKS_MARK, &depthBuffer.View);
	assert(r == VK_SUCCESS);
}

void VulkanTriangle::FindQueueFamilyIndices()
{
	// get info about present for each queue family
	VkBool32* supportsPresent = (VkBool32*)malloc(queueFamilyCount * sizeof(VkBool32));

	for (uint32_t i = 0; i < queueFamilyCount; i++)
	{
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[choosedPhysDevice], i, surface, &supportsPresent[i]);
	}

	graphicsQueueFamilyIndex = UINT32_MAX;
	presentQueueFamilyIndex = UINT32_MAX;

	// try to find queue family that supports graphics and present
	for (uint32_t i = 0; i < queueFamilyCount; ++i)
	{
		if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
		{
			if (graphicsQueueFamilyIndex == UINT32_MAX)
			{
				graphicsQueueFamilyIndex = i;
			}

			if (supportsPresent[i] == VK_TRUE)
			{
				graphicsQueueFamilyIndex = i;
				presentQueueFamilyIndex = i;

				break;
			}
		}
	}

	// if present index is not found
	if (presentQueueFamilyIndex == UINT32_MAX)
	{
		// try to find in separate families
		for (uint32_t i = 0; i < queueFamilyCount; ++i)
		{
			if (supportsPresent[i] == VK_TRUE)
			{
				presentQueueFamilyIndex = i;
				break;
			}
		}
	}

	free(supportsPresent);

	assert(graphicsQueueFamilyIndex != UINT32_MAX && presentQueueFamilyIndex != UINT32_MAX);
}

void VulkanTriangle::FindSupportedFormats()
{
	// get formats count
	uint32_t formatCount;
	VkResult r = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[choosedPhysDevice], surface, &formatCount, NULL);
	assert(r == VK_SUCCESS);
	assert(formatCount >= 1);
	
	VkSurfaceFormatKHR* surfFormats = (VkSurfaceFormatKHR*)malloc(formatCount * sizeof(VkSurfaceFormatKHR));

	// get formats
	r = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[choosedPhysDevice], surface, &formatCount, surfFormats);
	assert(r == VK_SUCCESS);
	assert(formatCount >= 1);

	// if surface doesn't have preferred format
	if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) 
	{
		surfaceFormat = VK_FORMAT_B8G8R8A8_UNORM;
	}
	else 
	{
		surfaceFormat = surfFormats[0].format;
	}

	free(surfFormats);
}

void VulkanTriangle::SetSurfaceCapabilities(VkSwapchainCreateInfoKHR& swapchainCreateInfo)
{
	VkSurfaceCapabilitiesKHR surfCapabilities;

	// get surface capabilities
	VkResult r = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevices[choosedPhysDevice], surface, &surfCapabilities);
	assert(r == VK_SUCCESS);

	uint32_t presentModeCount;
	r = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevices[choosedPhysDevice], surface, &presentModeCount, NULL);
	assert(r == VK_SUCCESS);

	// get present modes
	VkPresentModeKHR* presentModes = (VkPresentModeKHR*)malloc(presentModeCount * sizeof(VkPresentModeKHR));
	r = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevices[choosedPhysDevice], surface, &presentModeCount, presentModes);
	assert(r == VK_SUCCESS);

	VkExtent2D swapchainExtent;

	// if surface size is undefined
	if (surfCapabilities.currentExtent.width == UINT32_MAX)
	{
		swapchainExtent.width = WindowWidth;
		swapchainExtent.height = WindowHeight;

		if (swapchainExtent.width < surfCapabilities.minImageExtent.width) 
		{
			swapchainExtent.width = surfCapabilities.minImageExtent.width;
		}
		else if (swapchainExtent.width > surfCapabilities.maxImageExtent.width) 
		{
			swapchainExtent.width = surfCapabilities.maxImageExtent.width;
		}

		if (swapchainExtent.height < surfCapabilities.minImageExtent.height) 
		{
			swapchainExtent.height = surfCapabilities.minImageExtent.height;
		}
		else if (swapchainExtent.height > surfCapabilities.maxImageExtent.height)
		{
			swapchainExtent.height = surfCapabilities.maxImageExtent.height;
		}
	}
	else 
	{
		// if defined, swap chain size must match
		swapchainExtent = surfCapabilities.currentExtent;
	}

	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	VkSurfaceTransformFlagBitsKHR preTransform;
	if (surfCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) 
	{
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else 
	{
		preTransform = surfCapabilities.currentTransform;
	}

	// alpha mode
	VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR	};

	for (uint32_t i = 0; i < sizeof(compositeAlphaFlags) / sizeof(compositeAlphaFlags[0]); i++) 
	{
		if (surfCapabilities.supportedCompositeAlpha & compositeAlphaFlags[i]) 
		{
			compositeAlpha = compositeAlphaFlags[i];
			break;
		}
	}

	uint32_t desiredNumberOfSwapChainImages = surfCapabilities.minImageCount;

	// set properties
	swapchainCreateInfo.minImageCount = desiredNumberOfSwapChainImages;
	swapchainCreateInfo.imageFormat = surfaceFormat;
	swapchainCreateInfo.imageExtent.width = swapchainExtent.width;
	swapchainCreateInfo.imageExtent.height = swapchainExtent.height;
	swapchainCreateInfo.preTransform = preTransform;
	swapchainCreateInfo.compositeAlpha = compositeAlpha;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.presentMode = swapchainPresentMode;
}

void VulkanTriangle::CreatePipelineLayout(const VkDescriptorSetLayout *pSetLayouts, uint32_t setLayoutCount)
{
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = NULL;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = NULL;
	pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
	pipelineLayoutCreateInfo.pSetLayouts = pSetLayouts;

	VkResult r = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, TR_VK_ALLOCATION_CALLBACKS_MARK, &pipelineLayout);
	assert(r == VK_SUCCESS);
}

void VulkanTriangle::CreateDescriptorPool(const VkDescriptorPoolSize *poolSizes, uint32_t poolSizeCount)
{
	VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.pNext = NULL;
	// max number of descriptor sets, that can be allocated
	descriptorPoolInfo.maxSets = 1; 
	descriptorPoolInfo.poolSizeCount = poolSizeCount;
	descriptorPoolInfo.pPoolSizes = poolSizes;

	VkResult r = vkCreateDescriptorPool(device, &descriptorPoolInfo, TR_VK_ALLOCATION_CALLBACKS_MARK, &descriptorPool);
	assert(r == VK_SUCCESS);
}

void VulkanTriangle::AllocateDescriptorSets(const VkDescriptorSetLayout *descSetLayouts, uint32_t descriptorSetCount)
{
	VkDescriptorSetAllocateInfo descSetAllocInfo[1];
	descSetAllocInfo[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descSetAllocInfo[0].pNext = NULL;
	descSetAllocInfo[0].descriptorPool = this->descriptorPool;
	descSetAllocInfo[0].descriptorSetCount = descriptorSetCount;
	descSetAllocInfo[0].pSetLayouts = descSetLayouts;

	descriptorSets.resize(descriptorSetCount);
	VkResult r = vkAllocateDescriptorSets(device, descSetAllocInfo, descriptorSets.data());
	assert(r == VK_SUCCESS);
}

void VulkanTriangle::CreateRenderPass()
{
	VkAttachmentDescription attachmentDesc[2];

	// color
	attachmentDesc[0].format = surfaceFormat;
	attachmentDesc[0].samples = SampleCount;
	// clear at the start of render pass
	attachmentDesc[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// leave rendering result in this buffer
	attachmentDesc[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDesc[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDesc[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// initial layout is undefined
	attachmentDesc[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// final layout must be appropriate for present operation
	attachmentDesc[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachmentDesc[0].flags = 0;

	// depth
	attachmentDesc[1].format = depthBuffer.Format;
	attachmentDesc[1].samples = SampleCount;
	// clear at the start of render pass
	attachmentDesc[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDesc[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDesc[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDesc[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDesc[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// leave same as it will not be presented
	attachmentDesc[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachmentDesc[1].flags = 0;

	// create attachment ref for subpass
	VkAttachmentReference colorAttachRef = {};
	colorAttachRef.attachment = 0;
	// subpass occurs between initial and final layouts,
	// so choose optimal layout
	colorAttachRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachRef = {};
	depthAttachRef.attachment = 1;
	depthAttachRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpasses[1];

	subpasses[0] = {};
	// graphics pipeline type
	subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses[0].flags = 0;
	subpasses[0].inputAttachmentCount = 0;
	subpasses[0].pInputAttachments = NULL;
	subpasses[0].colorAttachmentCount = 1;
	subpasses[0].pColorAttachments = &colorAttachRef;
	subpasses[0].pResolveAttachments = NULL;
	subpasses[0].pDepthStencilAttachment = &depthAttachRef;
	subpasses[0].preserveAttachmentCount = 0;
	subpasses[0].pPreserveAttachments = NULL;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext = NULL;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachmentDesc;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = subpasses;
	renderPassInfo.dependencyCount = 0;
	renderPassInfo.pDependencies = NULL;

	VkResult r = vkCreateRenderPass(device, &renderPassInfo, TR_VK_ALLOCATION_CALLBACKS_MARK, &renderPass);
	assert(r == VK_SUCCESS);
}

void VulkanTriangle::CreateFramebuffers()
{
	VkImageView attachments[2];
	attachments[1] = depthBuffer.View;

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.pNext = NULL;
	framebufferInfo.renderPass = renderPass;
	framebufferInfo.attachmentCount = 2;
	framebufferInfo.pAttachments = attachments;
	framebufferInfo.width = WindowWidth;
	framebufferInfo.height = WindowHeight;
	framebufferInfo.layers = 1;

	// create framebuffer for each image
	framebuffers.resize(swapchainImageCount);

	for (int i = 0; i < swapchainImageCount; i++)
	{
		// set needed image
		attachments[0] = imageBuffers[i].View;

		VkResult r = vkCreateFramebuffer(device, &framebufferInfo, TR_VK_ALLOCATION_CALLBACKS_MARK, &framebuffers[i]);
		assert(r == VK_SUCCESS);
	}
}

std::vector<const char*> VulkanTriangle::GetRequiredInstanceExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers) 
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

std::vector<const char*> VulkanTriangle::GetRequiredDeviceExtensions()
{
	std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	return deviceExtensions;
}

void VulkanTriangle::DestroyVulkan()
{
	if (enableValidationLayers)
	{
		validationLayers.Destroy(vkInstance);
	}

	DestroyDepthBuffer();
	DestroySwapchain();
	DestroyDevice();
	DestroyInstance();
}

void VulkanTriangle::DestroyInstance()
{
	vkDestroyInstance(vkInstance, TR_VK_ALLOCATION_CALLBACKS_MARK);
}

void VulkanTriangle::DestroyDevice()
{
	vkDeviceWaitIdle(device);
	vkDestroyDevice(device, TR_VK_ALLOCATION_CALLBACKS_MARK);
}

void VulkanTriangle::DestroySwapchain()
{
	for (uint32_t i = 0; i < swapchainImageCount; i++)
	{
		vkDestroyImageView(device, imageBuffers[i].View, TR_VK_ALLOCATION_CALLBACKS_MARK);
	}

	vkDestroySwapchainKHR(device, swapchain, TR_VK_ALLOCATION_CALLBACKS_MARK);
}

void VulkanTriangle::DestroyDepthBuffer()
{
	vkDestroyImageView(device, depthBuffer.View, TR_VK_ALLOCATION_CALLBACKS_MARK);
	vkDestroyImage(device, depthBuffer.Image, TR_VK_ALLOCATION_CALLBACKS_MARK);
	vkFreeMemory(device, depthBuffer.Memory, TR_VK_ALLOCATION_CALLBACKS_MARK);
}

void VulkanTriangle::DestroyPipelineLayout()
{
	vkDestroyPipelineLayout(device, pipelineLayout, TR_VK_ALLOCATION_CALLBACKS_MARK);
}

void VulkanTriangle::DestroyDescriptorPool()
{
	vkDestroyDescriptorPool(device, descriptorPool, TR_VK_ALLOCATION_CALLBACKS_MARK);
}

void VulkanTriangle::DestroyRenderPass()
{
	vkDestroyRenderPass(device, renderPass, TR_VK_ALLOCATION_CALLBACKS_MARK);
}

void VulkanTriangle::DestroyFramebuffers()
{
	for (int i = 0; i < swapchainImageCount; i++)
	{
		vkDestroyFramebuffer(device, framebuffers[i], NULL);
	}
}

void VulkanTriangle::DestroyWindow()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Scene::Setup(const VulkanTriangle &t)
{
	//auto projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
	//
	//auto view = glm::lookAt(
	//	glm::vec3(-5, 3, -10), // Camera is at (-5,3,-10), in World Space
	//	glm::vec3(0, 0, 0),    // and looks at the origin
	//	glm::vec3(0, -1, 0)    // Head is up (set to 0,-1,0 to look upside-down)
	//);
	//
	//auto model = glm::mat4(1.0f);

	//auto clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
	//	0.0f, -1.0f, 0.0f, 0.0f,
	//	0.0f, 0.0f, 0.5f, 0.0f,
	//	0.0f, 0.0f, 0.5f, 1.0f);

	//auto mvp = clip * projection * view * model;

	//for (int i = 0; i < 4; i++)
	//{
	//	for (int j = 0; j < 4; j++)
	//	{
	//		MVP[i * 4 + j] = mvp[i][j];
	//	}
	//}

	MVP[0] = 2.15934f;
	MVP[1] = 0.279808f;
	MVP[2] = 0.432367f;
	MVP[3] = 0.431934f;
	
	MVP[4] = 0.0f;
	MVP[5] = 2.33173f;
	MVP[6] = -0.25942f;
	MVP[7] = -0.259161f;

	MVP[8] = -1.07967f;
	MVP[9] = 0.559615f;
	MVP[10] = 0.864733f;
	MVP[11] = 0.863868f;

	MVP[12] = 0.0f;
	MVP[13] = 0.0f;
	MVP[14] = 11.4873f;
	MVP[15] = 11.5758f;
}

void Scene::Destroy()
{}

bool Utils::GetMemoryType(const VkPhysicalDeviceMemoryProperties &deviceMemProperties, uint32_t memoryTypeBits, VkFlags requirementsMask, uint32_t& result)
{
	// for each memory type available for this device
	for (uint32_t i = 0; i < deviceMemProperties.memoryTypeCount; i++)
	{
		// if type is available
		if ((memoryTypeBits & 1) == 1)
		{
			if ((deviceMemProperties.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask)
			{
				result = i;
				return true;
			}
		}

		memoryTypeBits >>= 1;
	}

	return false;
}
