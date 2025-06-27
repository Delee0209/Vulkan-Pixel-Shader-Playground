#pragma once

// this file contain the include header, shared data structures and helper functions

#include <stdio.h>
#include <stdint.h>
#include <set> 
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <functional>
#include <chrono>

// vulkan header
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS // allowing designated initializers
//#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

// shaderc header
#include <shaderc/shaderc.hpp>

// GLFW header
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// GLM header
#define GLM_FORCE_RADIANS 
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// helper functions and structs
struct BufferInfo
{
    std::string name;
    uint32_t binding;
    vk::DeviceSize size;
    vk::Flags<vk::BufferUsageFlagBits> usage;
    vk::Flags<vk::MemoryPropertyFlagBits> memoryProperty;
    vk::DescriptorType descriptorType;
    void *data;
};

struct ImageInfo
{
    std::string name;
    uint32_t binding;
    glm::vec2 extent;
    vk::Format format;
    vk::Flags<vk::ImageUsageFlagBits> usage;
    vk::Flags<vk::MemoryPropertyFlagBits> memoryProperty;
    vk::DescriptorType descriptorType;
};

struct ShaderInfo
{
    std::string name;
    std::string filePath;
    std::string entryPoint;
    glm::vec3 globalDimension;
    glm::vec3 localDimension;
};

struct Buffer
{
    vk::raii::Buffer buffer;
    vk::raii::DeviceMemory memory;
    //vk::DeviceAddress address;
};

struct Image
{
    vk::raii::Image image;
    vk::raii::DeviceMemory memory;
    vk::raii::ImageView imageView;
    //vk::DeviceAddress address;
};

uint32_t findMemoryTypeIndex(vk::raii::PhysicalDevice &physicalDevice, const uint32_t &memoryTypeBits, const vk::MemoryPropertyFlags &properties)
{
    vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();
    for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if((memoryTypeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    return uint32_t(0);
}

Buffer createBuffer(vk::raii::Device &device, vk::raii::PhysicalDevice &physicalDevice, const vk::DeviceSize &size, const vk::Flags<vk::BufferUsageFlagBits> &usage, const vk::Flags<vk::MemoryPropertyFlagBits> &memoryProperty, const void *data = nullptr)
{
    vk::BufferCreateInfo bufferCreateInfo = {
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive
    };
    vk::raii::Buffer buffer(device, bufferCreateInfo);
    vk::MemoryRequirements memoryRequirements = buffer.getMemoryRequirements();
    vk::MemoryAllocateFlagsInfo allocateFlagsInfo = {
        .flags = vk::MemoryAllocateFlagBits::eDeviceAddress
    };
    vk::MemoryAllocateInfo allocateInfo = {
        .pNext = &allocateFlagsInfo,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memoryRequirements.memoryTypeBits, memoryProperty)
    };
    vk::raii::DeviceMemory memory(device, allocateInfo);
    buffer.bindMemory(*memory, 0);
    if(data)
    {
        void *mappedMemory = memory.mapMemory(0, size);
        memcpy(mappedMemory, data, size);
        memory.unmapMemory();
    }
    return Buffer{
        .buffer = std::move(buffer),
        .memory = std::move(memory)
        //.address = device.getBufferAddress({.buffer = buffer})
    };
}
void updateBuffer(const std::vector<Buffer> &buffers, const std::vector<BufferInfo> &bufferInfos, const std::string name)
{
    int index = -1;
    for(int i = 0; i < buffers.size(); i++)
    {
        if(bufferInfos[i].name == name)
        {
            index = i;
            break;
        }
    }
    if(index == -1) 
    {
        std::cout << "no buffer called " << name << " found\n";
        return;
    }
    void *mappedMemory = buffers[index].memory.mapMemory(0, bufferInfos[index].size);
    memcpy(mappedMemory, bufferInfos[index].data, bufferInfos[index].size);
    buffers[index].memory.unmapMemory();
}

Image createImage(vk::raii::Device &device, vk::raii::PhysicalDevice &physicalDevice, const vk::Extent3D extent, const vk::Format format, const vk::Flags<vk::ImageUsageFlagBits> &usage, const vk::Flags<vk::MemoryPropertyFlagBits> &memoryProperty, const vk::ImageTiling tiling = vk::ImageTiling::eOptimal)
{
    vk::ImageCreateInfo imageCreateInfo{
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined
    };
    vk::raii::Image image(device, imageCreateInfo);
    vk::MemoryRequirements memoryRequirements = image.getMemoryRequirements();
    vk::MemoryAllocateFlagsInfo allocateFlagsInfo = {
        .flags = vk::MemoryAllocateFlagBits::eDeviceAddress
    };
    vk::MemoryAllocateInfo allocateInfo = {
        .pNext = &allocateFlagsInfo,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memoryRequirements.memoryTypeBits, memoryProperty)
    };
    vk::raii::DeviceMemory memory(device, allocateInfo);
    image.bindMemory(*memory, 0);
    vk::ImageViewCreateInfo imageViewCreateInfo{
        .image = *image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange = vk::ImageSubresourceRange{
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    vk::raii::ImageView imageView(device, imageViewCreateInfo);
    return Image{
        .image = std::move(image),
        .memory = std::move(memory),
        .imageView = std::move(imageView)
        //.address = device.getBufferAddress({.buffer = buffer})
    };
}