#pragma once

// this file contain the parameter for the main program

# include "common.h"

// program setup
std::string programName = "PixelShaderPlayGround";
std::vector<const char*> requiredInstanceExtensions = {
    //
};
std::vector<const char*> requiredDeviceExtensions = {
    VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
    VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

glm::vec3 localDimension = glm::vec3(16, 16, 1);
const int width = 1280, height = 720;

// buffers
const vk::BufferUsageFlags uniformBufferUsageFlags = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
const vk::MemoryPropertyFlags uniformBufferMemoryPropertyFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
const vk::DescriptorType uniformBufferDescriptorType = vk::DescriptorType::eUniformBuffer;

const vk::BufferUsageFlags storageBufferUsageFlags = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
const vk::MemoryPropertyFlags storageBufferMemoryPropertyFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eDeviceLocal;
const vk::DescriptorType storageBufferDescriptorType = vk::DescriptorType::eStorageBuffer;

// images
const vk::ImageUsageFlags storageImageUsageFlags = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
const vk::MemoryPropertyFlags storageImageMemoryPropertyFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
const vk::DescriptorType storageImageDescriptorType = vk::DescriptorType::eStorageImage;

// buffer data structure
struct alignas(16) Parameters
{
    float time;
    glm::vec3 screenDimension;
} parameters;

// binding ID 0 is reserved for output image
std::vector<ImageInfo> imageInfos = {
    {.name = "outputImage", .binding = 0, .extent = glm::vec2(width, height), .format = vk::Format::eR8G8B8A8Unorm,        .usage = storageImageUsageFlags, .memoryProperty = storageImageMemoryPropertyFlags, .descriptorType = storageImageDescriptorType},
    //{.name = "image",       .binding = 1, .extent = glm::vec2(width, height), .format = vk::Format::eR32G32B32A32Sfloat,   .usage = storageImageUsageFlags, .memoryProperty = storageImageMemoryPropertyFlags, .descriptorType = storageImageDescriptorType},
};
int outputImageIndex = 0;

std::vector<BufferInfo> bufferInfos = {
    {.name = "parameters",  .binding = 2, .size = sizeof(parameters),   .usage = uniformBufferUsageFlags,   .memoryProperty = uniformBufferMemoryPropertyFlags,     .descriptorType = uniformBufferDescriptorType,  .data = &parameters},
    //{.name = "intArray",    .binding = 3, .size = sizeof(numA),         .usage = storageBufferUsageFlags,   .memoryProperty = storageBufferMemoryPropertyFlags,     .descriptorType = storageBufferDescriptorType,  .data = &numA},
};

const std::vector<ShaderInfo> shaderInfos = {
    //{.name = "shadertoyDefault",   .filePath = "../../shader/shadertoyDefault.comp",   .entryPoint = "main", .globalDimension = glm::vec3(width, height, 1), .localDimension = localDimension},
    //{.name = "shadertoyTest",   .filePath = "../../shader/shadertoyTest.comp",            .entryPoint = "main", .globalDimension = glm::vec3(width, height, 1), .localDimension = localDimension},
    {.name = "shadertoyAccretion", .filePath = "../../shader/shadertoyAccretion.comp", .entryPoint = "main", .globalDimension = glm::vec3(width, height, 1), .localDimension = localDimension},
    {.name = "shadertoyTunnel", .filePath = "../../shader/shadertoyTunnel.comp", .entryPoint = "main", .globalDimension = glm::vec3(width, height, 1), .localDimension = localDimension},
};

std::chrono::time_point<std::chrono::system_clock> start;
std::chrono::time_point<std::chrono::system_clock> end;

void initializeDeviceData()
{
    parameters.screenDimension = glm::vec3(width, height, 1);
    parameters.time = 0;
    start = std::chrono::system_clock::now();
}

void updateDeviceData(const std::vector<Buffer> &buffers, const std::vector<Image> &images)
{
    end = std::chrono::system_clock::now();
    parameters.time = std::chrono::duration<float>(end - start).count();
    updateBuffer(buffers, bufferInfos, "parameters");
}

void userInterface() /// TODO: IMGUI integration?
{
    //
}

void keyboardInput() /// TODO: keyboard input
{
    //
}