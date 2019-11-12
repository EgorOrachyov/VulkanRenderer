//
// Created by Egor Orachyov on 2019-11-02.
//

#include <VulkanRenderDevice.h>
#include <VulkanDefinitions.h>
#include <vulkan/vulkan.h>
#include <exception>
#include "VulkanUtils.h"

VulkanRenderDevice::VulkanRenderDevice(uint32 extensionsCount, const char *const *extensions) {

}

RenderDevice::ID VulkanRenderDevice::createVertexLayout(const std::vector<VertexBufferLayoutDesc> &vertexBuffersDesc) {
    VulkanVertexLayout layout;

    auto &vertBindings = layout.vkBindings;
    auto &vertAttributes = layout.vkAttributes;

    for (uint32 i = 0; i < vertexBuffersDesc.size(); i++) {
        const VertexBufferLayoutDesc &desc = vertexBuffersDesc[i];

        VkVertexInputBindingDescription bindingDesc;
        bindingDesc.binding = i;
        bindingDesc.inputRate = VulkanDefinitions::vertexInputRate(desc.usage);
        bindingDesc.stride = desc.stride;

        vertBindings.push_back(bindingDesc);

        for (uint32 j = 0; j < desc.attributes.size(); j++) {
            const VertexAttributeDesc &attr = desc.attributes[j];

            VkVertexInputAttributeDescription attrDesc;
            attrDesc.binding = bindingDesc.binding;
            attrDesc.format = VulkanDefinitions::dataFormat(attr.format);
            attrDesc.location = attr.location;
            attrDesc.offset = attr.offset;

            vertAttributes.push_back(attrDesc);
        }
    }

    return mVertexLayouts.move(layout);
}

void VulkanRenderDevice::destroyVertexLayout(RenderDevice::ID layout) {
    mVertexLayouts.remove(layout);
}

RenderDevice::ID VulkanRenderDevice::createVertexBuffer(BufferUsage type, uint32 size, const void *data) {
    VulkanVertexBuffer vertexBuffer = { };
    vertexBuffer.size = size;
    vertexBuffer.usage = type;

    VkBufferUsageFlagBits usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    if (type == BufferUsage::Dynamic) {
        VulkanUtils::createBuffer(context, size, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     vertexBuffer.vkBuffer, vertexBuffer.vkDeviceMemory
        );
        VulkanUtils::updateBufferMemory(context, vertexBuffer.vkDeviceMemory, 0, size, data);
    } else {
        VulkanUtils::createBufferLocal(context, data, size, usage, vertexBuffer.vkBuffer, vertexBuffer.vkDeviceMemory);
    }

    return mVertexBuffers.move(vertexBuffer);
}

RenderDevice::ID VulkanRenderDevice::createIndexBuffer(BufferUsage type, uint32 size, const void *data) {
    VulkanIndexBuffer indexBuffer = { };
    indexBuffer.size = size;
    indexBuffer.usage = type;

    VkBufferUsageFlagBits usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    if (type == BufferUsage::Dynamic) {
        VulkanUtils::createBuffer(context, size, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     indexBuffer.vkBuffer, indexBuffer.vkDeviceMemory
        );
        VulkanUtils::updateBufferMemory(context, indexBuffer.vkDeviceMemory, 0, size, data);
    } else {
        VulkanUtils::createBufferLocal(context, data, size, usage, indexBuffer.vkBuffer, indexBuffer.vkDeviceMemory);
    }

    return mIndexBuffers.move(indexBuffer);
}

void VulkanRenderDevice::updateVertexBuffer(RenderDevice::ID bufferId, uint32 size, uint32 offset, const void *data) {
    const VulkanVertexBuffer& buffer = mVertexBuffers.get(bufferId);
    const VkDeviceMemory &memory = buffer.vkDeviceMemory;

    if (buffer.usage != BufferUsage::Dynamic) {
        throw VulkanException("Attempt to update static vertex buffer");
    }

    if (size + offset > buffer.size) {
        throw VulkanException("Attempt to update out-of-buffer memory region for vertex buffer");
    }

    VulkanUtils::updateBufferMemory(context, memory, offset, size, data);
}

void VulkanRenderDevice::updateIndexBuffer(RenderDevice::ID bufferId, uint32 size, uint32 offset, const void *data) {
    const VulkanIndexBuffer& buffer = mIndexBuffers.get(bufferId);
    const VkDeviceMemory &memory = buffer.vkDeviceMemory;

    if (buffer.usage != BufferUsage::Dynamic) {
        throw VulkanException("Attempt to update static index buffer");
    }

    if (size + offset > buffer.size) {
        throw VulkanException("Attempt to update out-of-buffer memory region for index buffer");
    }

    VulkanUtils::updateBufferMemory(context, memory, offset, size, data);
}

void VulkanRenderDevice::destroyVertexBuffer(RenderDevice::ID bufferId) {
    const VkDevice &device = context.device;
    VulkanVertexBuffer& buffer = mVertexBuffers.get(bufferId);

    vkDestroyBuffer(device, buffer.vkBuffer, nullptr);
    vkFreeMemory(device, buffer.vkDeviceMemory, nullptr);

    mVertexBuffers.remove(bufferId);
}

void VulkanRenderDevice::destroyIndexBuffer(RenderDevice::ID bufferId) {
    const VkDevice &device = context.device;
    VulkanIndexBuffer& buffer = mIndexBuffers.get(bufferId);

    vkDestroyBuffer(device, buffer.vkBuffer, nullptr);
    vkFreeMemory(device, buffer.vkDeviceMemory, nullptr);

    mVertexBuffers.remove(bufferId);
}

RenderDevice::ID VulkanRenderDevice::createTexture(const RenderDevice::TextureDesc &textureDesc) {
    VkFormat format = VulkanDefinitions::dataFormat(textureDesc.format);
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageType imageType = VulkanDefinitions::imageType(textureDesc.type);
    VkImageViewType viewType = VulkanDefinitions::imageViewType(textureDesc.type);

    if (textureDesc.usageFlags & (uint32) TextureUsageBit::ShaderSampling) {
        VulkanImageObject imo;

        VulkanUtils::createTextureImage(context,
                                   textureDesc.data,
                                   textureDesc.width, textureDesc.height, textureDesc.depth,
                                   imageType, format, VK_IMAGE_TILING_OPTIMAL,
                                   imo.image, imo.imageMemory, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        VkImageSubresourceRange subresourceRange;
        subresourceRange.aspectMask = aspectMask;
        // TODO: mipmaps
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;

        VulkanUtils::createImageView(context, imo.imageView, imo.image, viewType, format, subresourceRange);

        return mImageObjects.move(imo);
    } else if (textureDesc.usageFlags & (uint32) TextureUsageBit::DepthStencilAttachment) {
        VulkanImageObject depthStencil;

        // get properties of depth stencil format
        const VkFormatProperties &properties = VulkanUtils::getDeviceFormatProperties(context, format);

        VkImageTiling tiling;

        if (properties.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            tiling = VK_IMAGE_TILING_LINEAR;
        } else if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            tiling = VK_IMAGE_TILING_OPTIMAL;
        } else {
            throw VulkanException("Unsupported depth format");
        }

        VulkanUtils::createImage(context, textureDesc.width, textureDesc.height, textureDesc.depth,
                            imageType, format, tiling, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                // depth stencil buffer is device local
                // TODO: make visible from cpu
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                            depthStencil.image, depthStencil.imageMemory);

        VkComponentMapping components;
        components.r = VK_COMPONENT_SWIZZLE_R;
        components.g = VK_COMPONENT_SWIZZLE_G;
        components.b = VK_COMPONENT_SWIZZLE_B;
        components.a = VK_COMPONENT_SWIZZLE_A;

        VkImageSubresourceRange subresourceRange;
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;

        VulkanUtils::createImageView(context, depthStencil.imageView, depthStencil.image,
                                viewType, format, subresourceRange, components);

        return mImageObjects.move(depthStencil);
    } else if (textureDesc.usageFlags & (uint32) TextureUsageBit::ColorAttachment) {
        throw VulkanException("Color attachments must created with swapchain");
    } else {
        throw VulkanException("Failed to create texture object");
    }
}

void VulkanRenderDevice::destroyTexture(RenderDevice::ID textureId) {
    const VkDevice &device = context.device;
    VulkanImageObject &imo = mImageObjects.get(textureId);

    vkDestroyImageView(device, imo.imageView, nullptr);
    vkDestroyImage(device, imo.image, nullptr);
    vkFreeMemory(device, imo.imageMemory, nullptr);

    mImageObjects.remove(textureId);
}

RenderDevice::ID VulkanRenderDevice::createSampler(const RenderDevice::SamplerDesc &samplerDesc) {
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.minFilter = VulkanDefinitions::filter(samplerDesc.min);
    samplerInfo.magFilter = VulkanDefinitions::filter(samplerDesc.mag);
    samplerInfo.addressModeU = VulkanDefinitions::samplerAddressMode(samplerDesc.u);
    samplerInfo.addressModeV = VulkanDefinitions::samplerAddressMode(samplerDesc.v);
    samplerInfo.addressModeW = VulkanDefinitions::samplerAddressMode(samplerDesc.w);
    samplerInfo.anisotropyEnable = samplerDesc.useAnisotropy ? VK_TRUE : VK_FALSE;
    samplerInfo.maxAnisotropy = samplerDesc.anisotropyMax;
    samplerInfo.borderColor = VulkanDefinitions::borderColor(samplerDesc.color);
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    VkSampler sampler;
    VkResult r = vkCreateSampler(context.device, &samplerInfo, nullptr, &sampler);

    if (r != VK_SUCCESS) {
        throw VulkanException("Failed to create sampler object");
    }

    return mSamplers.add(sampler);
}

void VulkanRenderDevice::destroySampler(RenderDevice::ID samplerId) {
    vkDestroySampler(context.device, mSamplers.get(samplerId), nullptr);
    mSamplers.remove(samplerId);
}

RenderDevice::ID VulkanRenderDevice::getSurface(const std::string &surfaceName) {
    for (auto i = mSurfaces.begin(); i != mSurfaces.end(); ++i) {
        auto& window = *i;
        if (window.name == surfaceName) {
            return i.getID();
        }
    }
    return INVALID;
}

void VulkanRenderDevice::getSurfaceSize(RenderDevice::ID surface, uint32 &width, uint32 &height) {
    auto& window = mSurfaces.get(surface);

    width = window.width;
    height = window.height;
}

RenderDevice::ID VulkanRenderDevice::createFramebufferFormat(const std::vector<RenderDevice::FramebufferAttachmentDesc> &attachments) {
    std::vector<VkAttachmentDescription> attachmentDescriptions(attachments.size());
    std::vector<VkAttachmentReference> attachmentReferences(attachments.size());

    bool useDepthStencil = false;
    VkAttachmentReference depthStencilAttachmentReference;

    for (uint32 i = 0; i < attachments.size(); i++) {
        const RenderDevice::FramebufferAttachmentDesc& attachment = attachments[i];
        VkImageLayout layout = VulkanDefinitions::imageLayout(attachment.type);

        VkAttachmentDescription description = {};
        description.format = VulkanDefinitions::dataFormat(attachment.format);
        description.samples = VulkanDefinitions::samplesCount(attachment.samples);
        description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        description.finalLayout = layout;

        VkAttachmentReference reference = {};
        reference.attachment = i;
        reference.layout = layout;

        if (attachment.type == AttachmentType::DepthStencil) {
            if (useDepthStencil) {
                throw VulkanException("An attempt to use more then 1 depth stencil attachment");
            } else {
                useDepthStencil = true;
                depthStencilAttachmentReference = reference;
            }
        } else {
            attachmentReferences.push_back(reference);
        }

        attachmentDescriptions.push_back(description);
    }

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = (uint32) attachmentReferences.size();
    subpass.pColorAttachments = attachmentReferences.data();
    if (useDepthStencil) subpass.pDepthStencilAttachment = &depthStencilAttachmentReference;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = (uint32) attachmentDescriptions.size();
    renderPassInfo.pAttachments = attachmentDescriptions.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkRenderPass renderPass;

    if (vkCreateRenderPass(context.device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw VulkanException("Failed to create render pass");
    }

    VulkanFrameBufferFormat format = {};
    format.renderPass = renderPass;

    return mFrameBufferFormats.move(format);
}

void VulkanRenderDevice::destroyFramebufferFormat(RenderDevice::ID framebufferFormat) {
    vkDestroyRenderPass(context.device, mFrameBufferFormats.get(framebufferFormat).renderPass, nullptr);
    mFrameBufferFormats.remove(framebufferFormat);
}
