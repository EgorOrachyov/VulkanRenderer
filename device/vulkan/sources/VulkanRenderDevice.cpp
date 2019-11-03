//
// Created by Egor Orachyov on 2019-11-02.
//

#include "include/VulkanRenderDevice.h"
#include "include/VulkanDefinitions.h"
#include <vulkan/vulkan.h>
#include <exception>

RenderDevice::ID VulkanRenderDevice::createVertexLayout(const std::vector<VertexBufferLayoutDesc> &vertexBuffersDesc) {
    VertexLayoutBatch batch;

    auto& vertBindings = batch.vertBindings;
    auto& vertAtributes = batch.vertAtributes;

    for (size_t i = 0; i < vertexBuffersDesc.size(); i++) {
        const VertexBufferLayoutDesc& desc = vertexBuffersDesc[i];

        VkVertexInputBindingDescription bindingDesc;
        bindingDesc.binding = (uint32_t)i;
        bindingDesc.inputRate = VulkanDefinitions::vertexInputRate(desc.usage);
        bindingDesc.stride = (uint32_t)desc.stride;

        vertBindings.push_back(bindingDesc);

        for (size_t j = 0; j < desc.attributes.size(); j++)
        {
            const VertexAttributeDesc &attr = desc.attributes[j];

            VkVertexInputAttributeDescription attrDesc;
            attrDesc.binding = bindingDesc.binding;
            attrDesc.format = VulkanDefinitions::dataFormat(attr.format);
            attrDesc.location = attr.location;
            attrDesc.offset = attr.offset;

            vertAtributes.push_back(attrDesc);
        }
    }

    return vertexLayoutBatches.add(batch);
}

void VulkanRenderDevice::destroyVertexLayout(RenderDevice::ID layout) {
    vertexLayoutBatches.remove(layout);
}

RenderDevice::ID VulkanRenderDevice::createVertexBuffer(BufferUsage usage, uint32 size, const void *data) {
    const VkDevice device = context.getDevice();

    VertexBufferObject vertexBufferObj = {};

    if (usage == BufferUsage::Dynamic)
    {
        // create vertex buffer and allocate memory
        _createBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                // to be visible from host for updating buffer memory
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                vertexBufferObj.buffer, vertexBufferObj.memory);

        // copy data to allocated memory
    }
    else
    {
        // allocate in device local memory
        // and set data
        _createBufferLocal(data, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                vertexBufferObj.buffer, vertexBufferObj.memory);
    }

    return vertexBuffers.add(vertexBufferObj);
}

void VulkanRenderDevice::updateVertexBuffer(RenderDevice::ID bufferId, uint32 size, uint32 offset, const void *data) {
    const VkDeviceMemory &bufferMemory = vertexBuffers.getPtr(bufferId)->memory;
    _updateBufferMemory(bufferMemory, data, size, offset);
}

void VulkanRenderDevice::destroyVertexBuffer(RenderDevice::ID bufferId) {
    const VkDevice &device = context.getDevice();
    VertexBufferObject *vb = vertexBuffers.getPtr(bufferId);

    vkDestroyBuffer(device, vb->buffer, nullptr);
    vkFreeMemory(device, vb->memory, nullptr);
}

void VulkanRenderDevice::_createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                                 VkBuffer &outBuffer, VkDeviceMemory &outBufferMemory) {

    const VkDevice &device = context.getDevice();

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = (VkDeviceSize)size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult r = vkCreateBuffer(device, &bufferInfo, nullptr, &outBuffer);
    if (r != VK_SUCCESS)
    {
        throw std::exception("Vulkan::Can't create buffer for vertex data");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, outBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = context.getMemoryTypeIndex(memRequirements.memoryTypeBits, properties);

    r = vkAllocateMemory(device, &allocInfo, nullptr, &outBufferMemory);
    if (r != VK_SUCCESS)
    {
        throw std::exception("Vulkan::Can't allocate memory for vertex buffer");
    }

    r = vkBindBufferMemory(device, outBuffer, outBufferMemory, 0);
    if (r != VK_SUCCESS)
    {
        throw std::exception("Vulkan::Can't bind buffer memory for vertex buffer");
    }
}

void VulkanRenderDevice::_createBufferLocal(const void *data, VkDeviceSize size, VkBufferUsageFlags usage,
                                       VkBuffer &outBuffer, VkDeviceMemory &outBufferMemory) {

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    const VkDevice &device = context.getDevice();

    // create staging buffer, visible to host
    _createBuffer(size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);

    // copy data to staging buffer memory
    _updateBufferMemory(stagingBufferMemory, data, size, 0);

    // create actual buffer
    _createBuffer(size,
            usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            outBuffer,outBufferMemory);

    _copyBuffer(context.getCommandPool(), context.getTransferQueue(), stagingBuffer, outBuffer, size);

    // delete staging buffer
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void VulkanRenderDevice::_copyBuffer(VkCommandPool commandPool, VkQueue queue, VkBuffer srcBuffer, VkBuffer dstBuffer,
                                     VkDeviceSize size) {
    const VkDevice &device = context.getDevice();

    // TODO: create _beginTempCommandBuffer and _endTempCommandBuffer
    VkCommandBuffer commandBuffer; // = _beginTempCommandBuffer(device, commandPool);

    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    copyRegion.dstOffset = 0;
    copyRegion.srcOffset = 0;

    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    // _endTempCommandBuffer(device, queue, commandPool, commandBuffer);
}

void VulkanRenderDevice::_updateBufferMemory(const VkDeviceMemory &bufferMemory, const void *data, VkDeviceSize size, VkDeviceSize offset) {
    const VkDevice &device = context.getDevice();

    void* mappedData;
    vkMapMemory(device, bufferMemory, offset, size, 0, &mappedData);
    memcpy(mappedData, data, (size_t)size);

    vkUnmapMemory(device, bufferMemory);
}