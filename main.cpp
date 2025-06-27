#include "common.h"
#include "setup.h"

int main()
{
    // prepare buffer/image content
    initializeDeviceData();

    // setup window 
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(width, height, programName.c_str(), nullptr, nullptr);
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> glfwInstanceExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    for(int i = 0; i < glfwInstanceExtensions.size(); i++)
    {
        requiredInstanceExtensions.emplace_back(glfwInstanceExtensions[i]);
    }

    // create instance
    vk::raii::Context context;
    const vk::ApplicationInfo applicationInfo{ 
        .pApplicationName = programName.c_str(),
        .applicationVersion = 1,
        .pEngineName = nullptr,
        .engineVersion = 1,
        .apiVersion = VK_API_VERSION_1_3
    };
    vk::InstanceCreateInfo instanceCreateInfo{
        .pApplicationInfo = &applicationInfo,
        .enabledExtensionCount = (uint32_t) requiredInstanceExtensions.size(),
        .ppEnabledExtensionNames = requiredInstanceExtensions.data()
    };
    vk::raii::Instance instance(context, instanceCreateInfo);

    // set up window surface
    VkSurfaceKHR rawSurface;
    glfwCreateWindowSurface(*instance, window, nullptr, &rawSurface);
    vk::raii::SurfaceKHR surface(instance, rawSurface);

    // selecting physical device
    std::vector<vk::raii::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    for(const vk::raii::PhysicalDevice &deviceCandidate : physicalDevices)
    {
        std::vector<vk::ExtensionProperties> availableExtensions = deviceCandidate.enumerateDeviceExtensionProperties();
        std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());
        for(const vk::ExtensionProperties &extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }
        if(requiredExtensions.empty())
        {
            physicalDevice = deviceCandidate;
            break;
        }
    }

    // finding and select one queue family that support both compute and graphics
    uint32_t queueFamily = (uint32_t)-1;
    {
        std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();
        for(uint32_t i = 0; i < queueFamilies.size(); i++)
        {
            bool supportsCompute = (queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute) == vk::QueueFlagBits::eCompute;
            bool supportsPresent = true;// = physicalDevice.getSurfaceSupportKHR(i, *surface);
            if(supportsCompute && supportsPresent)
            {
                queueFamily = i;
            }
        }
    }

    // create logical device
    float queuePriority = 1.f;
    vk::DeviceQueueCreateInfo queueCreateInfo{
        .queueFamilyIndex = queueFamily, 
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{
        queueCreateInfo
    };
    vk::DeviceCreateInfo deviceCreateInfo{
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size()),
        .ppEnabledExtensionNames = requiredDeviceExtensions.data(),
        .pEnabledFeatures = nullptr
    };
    vk::raii::Device device(physicalDevice, deviceCreateInfo);

    // setup swapchain
    uint32_t imageCount = 2; // keep it minimal
    vk::SwapchainCreateInfoKHR swapchainCreateInfo{
        .surface = *surface,
        .minImageCount = imageCount,
        .imageFormat = imageInfos[outputImageIndex].format,
        .imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
        .imageExtent = vk::Extent2D{(uint32_t)imageInfos[outputImageIndex].extent.x, (uint32_t)imageInfos[outputImageIndex].extent.y},
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = vk::PresentModeKHR::eFifo,
        .clipped = true
    };
    vk::raii::SwapchainKHR swapchain(device, swapchainCreateInfo);
    std::vector<vk::Image> swapchainImages = swapchain.getImages();
    std::vector<vk::raii::ImageView> swapchainImagesViews;
    for(const auto &image : swapchainImages)
    {
        vk::ImageViewCreateInfo swapchainImageViewCreateInfo{
            .image = image,
            .viewType = vk::ImageViewType::e2D,
            .format = vk::Format::eR8G8B8A8Unorm,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        swapchainImagesViews.emplace_back(device, swapchainImageViewCreateInfo);
    }

    // create buffers
    std::vector<Buffer> buffers;
    for(int i = 0; i < bufferInfos.size(); i++)
    {
        buffers.emplace_back(createBuffer(device, physicalDevice, bufferInfos[i].size, bufferInfos[i].usage, bufferInfos[i].memoryProperty, bufferInfos[i].data));
    }
    std::vector<vk::DescriptorBufferInfo> descriptorBufferInfos;
    for(int i = 0; i < bufferInfos.size(); i++)
    {
        descriptorBufferInfos.emplace_back(vk::DescriptorBufferInfo{
            .buffer = *buffers[i].buffer,
            .offset = 0,
            .range = bufferInfos[i].size
        });
    }

    // create images
    std::vector<Image> images;
    for(int i = 0; i < imageInfos.size(); i++)
    {
        images.emplace_back(createImage(device, physicalDevice, {(uint32_t)imageInfos[i].extent.x, (uint32_t)imageInfos[i].extent.y, 1}, imageInfos[i].format, imageInfos[i].usage, imageInfos[i].memoryProperty));
    }
    std::vector<vk::DescriptorImageInfo> descriptorImageInfos;
    for(int i = 0; i < imageInfos.size(); i++)
    {
        descriptorImageInfos.emplace_back(vk::DescriptorImageInfo{
            .sampler = nullptr,
            .imageView = *images[i].imageView,
            .imageLayout = vk::ImageLayout::eGeneral
        });
    }

    std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings;
    for(int i = 0; i < bufferInfos.size(); i++)
    {
        descriptorSetLayoutBindings.emplace_back(vk::DescriptorSetLayoutBinding{
            .binding = bufferInfos[i].binding,
            .descriptorType = bufferInfos[i].descriptorType,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute
        });
    }
    for(int i = 0; i < imageInfos.size(); i++)
    {
        descriptorSetLayoutBindings.emplace_back(vk::DescriptorSetLayoutBinding{
            .binding = imageInfos[i].binding,
            .descriptorType = imageInfos[i].descriptorType,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute
        });
    }

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{
        .flags = {},
        .bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size()),
        .pBindings = descriptorSetLayoutBindings.data()
    };
    vk::raii::DescriptorSetLayout descriptorSetLayout(device, descriptorSetLayoutCreateInfo);

    // create descriptor pool
    std::vector<vk::DescriptorPoolSize> descriptorPoolSizes{
        vk::DescriptorPoolSize{
            .type = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = static_cast<uint32_t>(bufferInfos.size())
        },
        vk::DescriptorPoolSize{
            .type = vk::DescriptorType::eStorageImage,
            .descriptorCount = static_cast<uint32_t>(imageInfos.size())
        },
    };
    vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 1,
        .poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size()),
        .pPoolSizes = descriptorPoolSizes.data()
    };
    vk::raii::DescriptorPool descriptorPool(device, descriptorPoolCreateInfo);
    vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{
        .descriptorPool = *descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &*descriptorSetLayout
    };
    auto descriptorSets = vk::raii::DescriptorSets(device, descriptorSetAllocateInfo);

    // write descriptor
    std::vector<vk::WriteDescriptorSet> writes;
    for(int i = 0; i < bufferInfos.size(); i++)
    {
        writes.emplace_back(vk::WriteDescriptorSet{
            .dstSet = descriptorSets[0],
            .dstBinding = bufferInfos[i].binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = bufferInfos[i].descriptorType,
            .pBufferInfo = &descriptorBufferInfos[i]
        });
    }
    for(int i = 0; i < imageInfos.size(); i++)
    {
        writes.emplace_back(vk::WriteDescriptorSet{
            .dstSet = descriptorSets[0],
            .dstBinding = imageInfos[i].binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = imageInfos[i].descriptorType,
            .pImageInfo = &descriptorImageInfos[i]
        });
    }
    device.updateDescriptorSets(writes, nullptr);

    // compile GLSL shader file to SPIR-V and setup shader module
    std::vector<vk::raii::ShaderModule> shaderModules;
    for(int i = 0; i < shaderInfos.size(); i++)
    {
        std::ifstream shaderFile(shaderInfos[i].filePath);
        std::stringstream stringBuffer;
        stringBuffer << shaderFile.rdbuf();
        std::string glslCode = stringBuffer.str();
        const auto compiled = shaderc::Compiler().CompileGlslToSpv(glslCode, shaderc_compute_shader, "temp.comp");
        const std::vector<uint32_t> spirvCode(compiled.cbegin(), compiled.cend());
        vk::ShaderModuleCreateInfo shaderModuleCreateInfo{
            .flags = {},
            .codeSize = std::distance(spirvCode.begin(), spirvCode.end()) * sizeof(uint32_t),
            .pCode = spirvCode.data()
        };
        shaderModules.emplace_back(device.createShaderModule(shaderModuleCreateInfo));
    }

    // setup compute pipeline
    std::vector<vk::raii::PipelineLayout> pipelineLayouts;
    std::vector<vk::raii::Pipeline> pipelines;
    for(int i = 0; i < shaderInfos.size(); i++)
    {
        vk::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfo{
            .flags = {},
            .stage = vk::ShaderStageFlagBits::eCompute,
            .module = *shaderModules[i],
            .pName = shaderInfos[i].entryPoint.c_str()
        };
        vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{
            .flags = {},
            .setLayoutCount = 1,
            .pSetLayouts = &*descriptorSetLayout
        };
        pipelineLayouts.emplace_back(device, pipelineLayoutCreateInfo);
        const vk::ComputePipelineCreateInfo pipelineCreateInfo{
            .flags = {},
            .stage = pipelineShaderStageCreateInfo,
            .layout = *pipelineLayouts[i]
        };
        pipelines.emplace_back(device.createComputePipeline(device.createPipelineCache({}), pipelineCreateInfo));
    }

    // setup command buffer
    vk::CommandPoolCreateInfo commandPoolCreateInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = static_cast<uint32_t>(queueFamily)
    };
    auto pool = device.createCommandPool(commandPoolCreateInfo);
    const vk::CommandBufferAllocateInfo allocateInfo{
        .commandPool = *pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = static_cast<uint32_t>(swapchainImages.size())
    };
    auto cmdBuffers = device.allocateCommandBuffers(allocateInfo);

    // imageLayout transition for each storage image
    cmdBuffers[0].begin(vk::CommandBufferBeginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    });

    for(int i = 0; i < imageInfos.size(); i++)
    {
        vk::ImageMemoryBarrier imageMemoryBarrier{
            .srcAccessMask = {},
            .dstAccessMask = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,
            .oldLayout = vk::ImageLayout::eUndefined,
            .newLayout = vk::ImageLayout::eGeneral,
            .image = *images[i].image,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        cmdBuffers[0].pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eComputeShader, {}, nullptr, nullptr, imageMemoryBarrier);
    }
    cmdBuffers[0].end();
    device.getQueue(queueFamily, 0).submit(vk::SubmitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &(*cmdBuffers[0])
    });
    device.waitIdle();

    // dispatch compute pipeline
    vk::SemaphoreCreateInfo semaphoreCreateInfo{};
    vk::raii::Semaphore imageAvailableSemaphore(device, semaphoreCreateInfo);
    vk::raii::Semaphore renderFinishedSemaphore(device, semaphoreCreateInfo);

    vk::MemoryBarrier memoryBarrier{
        .srcAccessMask = vk::AccessFlagBits::eShaderWrite,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead
    };
    vk::ImageCopy regions{
        .srcSubresource = {
            vk::ImageAspectFlagBits::eColor,
            0, 0, 1
        },
        .srcOffset = {0, 0, 0},
        .dstSubresource = {
            vk::ImageAspectFlagBits::eColor,
            0, 0, 1
        },
        .dstOffset = {0, 0, 0},
        .extent = vk::Extent3D{width, height, 1}
    };
    std::array<vk::PipelineStageFlags, 1> waitStages = {
        vk::PipelineStageFlagBits::eComputeShader
    };

    for(int imageIndex = 0; imageIndex < swapchainImages.size(); imageIndex++)
    {
        cmdBuffers[imageIndex].begin(vk::CommandBufferBeginInfo{});

        for(int i = 0; i < shaderInfos.size(); i++)
        {
            cmdBuffers[imageIndex].bindPipeline(vk::PipelineBindPoint::eCompute, *pipelines[i]);
            cmdBuffers[imageIndex].bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayouts[i], 0, *descriptorSets[0], {});

            glm::vec3 dispatchDimension = (shaderInfos[i].globalDimension + shaderInfos[i].localDimension - 1.f) / shaderInfos[i].localDimension;
            cmdBuffers[imageIndex].dispatch(static_cast<uint32_t>(dispatchDimension.x), static_cast<uint32_t>(dispatchDimension.y), static_cast<uint32_t>(dispatchDimension.z));

            cmdBuffers[imageIndex].pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, memoryBarrier, nullptr, nullptr);
        }

        cmdBuffers[imageIndex].pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer,
            {},
            nullptr, nullptr,
            vk::ImageMemoryBarrier{
                .srcAccessMask = {},
                .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
                .oldLayout = vk::ImageLayout::eUndefined,  // If first use this frame
                .newLayout = vk::ImageLayout::eTransferDstOptimal,
                .image = swapchainImages[imageIndex],
                .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
            }
        );
        cmdBuffers[imageIndex].copyImage(*images[outputImageIndex].image, vk::ImageLayout::eGeneral, swapchainImages[imageIndex], vk::ImageLayout::eTransferDstOptimal, regions);
        vk::ImageMemoryBarrier imageMemoryBarrier{
            .srcAccessMask = {},
            .dstAccessMask = vk::AccessFlagBits::eMemoryRead,
            .oldLayout = vk::ImageLayout::eTransferDstOptimal,
            .newLayout = vk::ImageLayout::ePresentSrcKHR,
            .image = swapchainImages[imageIndex],
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        cmdBuffers[imageIndex].pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe, {}, nullptr, nullptr, imageMemoryBarrier);

        cmdBuffers[imageIndex].end();
    }

    // render loop
    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        auto [result, imageIndex] = swapchain.acquireNextImage(UINT64_MAX, *imageAvailableSemaphore, nullptr);

        device.getQueue(queueFamily, 0).submit(vk::SubmitInfo{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*imageAvailableSemaphore,
            .pWaitDstStageMask = waitStages.data(),
            .commandBufferCount = 1,
            .pCommandBuffers = &(*cmdBuffers[imageIndex]),
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &*renderFinishedSemaphore
        });

        vk::PresentInfoKHR presentInfo{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*renderFinishedSemaphore,
            .swapchainCount = 1,
            .pSwapchains = &*swapchain,
            .pImageIndices = &imageIndex
        };
        device.getQueue(queueFamily, 0).presentKHR(presentInfo);

        device.waitIdle();

        updateDeviceData(buffers, images);
    }

    return 0;
}
