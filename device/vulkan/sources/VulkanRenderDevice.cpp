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
    VkImageUsageFlags usageFlags = VulkanDefinitions::imageUsageFlags(textureDesc.usageFlags);

    VulkanTextureObject texture = {};
    texture.type = imageType;
    texture.format = format;
    texture.usageFlags = usageFlags;
    texture.width = textureDesc.width;
    texture.height = textureDesc.height;
    texture.depth = textureDesc.depth;
    texture.mipmaps = textureDesc.mipmaps;

    if (usageFlags & (uint32)VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT)
    {
        if (usageFlags & (uint32)VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT &&
            usageFlags & (uint32)VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT == 0) {

            // if texture can be sampled in shader and it's a color attachment

            // TODO

        } else if (usageFlags & (uint32)VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT == 0 &&
            usageFlags & (uint32)VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {

            // if texture can be sampled in shader and it's a depth stencil attachment

            // TODO

        } else if (usageFlags & (uint32)VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT == 0 &&
            usageFlags & (uint32)VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT == 0) {

            // if texture can be sampled in shader and it's not an attachment

            // create texture image with mipmaps and allocate memory
            VulkanUtils::createTextureImage(context, textureDesc.data,
                textureDesc.width, textureDesc.height, textureDesc.depth, textureDesc.mipmaps,
                imageType, format, VK_IMAGE_TILING_OPTIMAL,
                texture.image, texture.imageMemory, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            // create image view
            VkImageSubresourceRange subresourceRange;
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = textureDesc.mipmaps;
            subresourceRange.baseArrayLayer = 0;
            subresourceRange.layerCount = 1;

            VulkanUtils::createImageView(context, 
                texture.imageView, texture.image, 
                viewType, format, subresourceRange);

            return mTextureObjects.move(texture);

        } else {

            VulkanException("VulkanRenderDevice::Texture can't be color and depth stencil attachment at the same time");
        }

    } else if (usageFlags & (uint32)VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT &&
        usageFlags & (uint32)VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT == 0) {

        // if depth stencil attahment

        VulkanUtils::createDepthStencilBuffer(context, textureDesc.width, textureDesc.height, textureDesc.depth,
            imageType, format, viewType, texture.image, texture.imageMemory, texture.imageView);

        return mTextureObjects.move(texture);

    } else if (usageFlags & (uint32)VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT &&
        usageFlags & (uint32)VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT == 0) {

        // if color attachment

        // TODO
        texture.image = VK_NULL_HANDLE;
        texture.imageMemory = VK_NULL_HANDLE;
        texture.imageView = VK_NULL_HANDLE;

        return mTextureObjects.move(texture);
    } else {
        throw VulkanException("Texture can't be color and depth stencil attachment at the same time");
    }
}

void VulkanRenderDevice::destroyTexture(RenderDevice::ID textureId) {
    const VkDevice &device = context.device;
    VulkanTextureObject &imo = mTextureObjects.get(textureId);

    vkDestroyImageView(device, imo.imageView, nullptr);
    vkDestroyImage(device, imo.image, nullptr);
    vkFreeMemory(device, imo.imageMemory, nullptr);

    mTextureObjects.remove(textureId);
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
    samplerInfo.mipmapMode = VulkanDefinitions::samplerMipmapMode(samplerDesc.mipmapMode);
    samplerInfo.mipLodBias = samplerDesc.mipLodBias;
    samplerInfo.minLod = samplerDesc.minLod;
    samplerInfo.maxLod = samplerDesc.maxLod;

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

RenderDevice::ID VulkanRenderDevice::createFramebuffer(const std::vector<RenderDevice::ID>& attachmentIds,
    RenderDevice::ID framebufferFormatId) {

    // get framebuffer attachemnts info
    std::vector<VkImageView> framebufferAttchViews;
    uint32 width, height;

    for (size_t i = 0; i < attachmentIds.size(); i++) {
        VulkanTextureObject& att = mTextureObjects.get(attachmentIds[i]);

        // also, check widths and heights
        if (i == 0)
        {
            width = att.width;
            height = att.height;
        }
        else if (width != att.width || height != att.height)
        {
            throw VulkanException("Framebuffer attachments must be same size");
        }

        framebufferAttchViews.push_back(att.imageView);
    }

    VkFramebufferCreateInfo framebufferInfo;
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = NULL;
    framebufferInfo.flags = 0;
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.layers = 1;
    framebufferInfo.attachmentCount = framebufferAttchViews.size();
    framebufferInfo.pAttachments = framebufferAttchViews.data();
    // get render pass from framebuffer format
    framebufferInfo.renderPass = mFrameBufferFormats.get(framebufferFormatId).renderPass;

    VkFramebuffer framebuffer;
    VkResult r = vkCreateFramebuffer(context.device, &framebufferInfo, nullptr, &framebuffer);

    if (r != VK_SUCCESS)
    {
        throw VulkanException("Can't create framebuffer");
    }

    return mFramebuffers.add(framebuffer);
}

void VulkanRenderDevice::destroyFramebuffer(RenderDevice::ID framebufferId) {
    VkFramebuffer framebuffer = mFramebuffers.get(framebufferId);
    vkDestroyFramebuffer(context.device, framebuffer, nullptr);

    mFramebuffers.remove(framebufferId);
}