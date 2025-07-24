# Vulkan Pixel Shader Playground
A lightweight playground for quickly setting up a pixel GLSL shader pipeline (similar to Shadertoy-style shaders!).
- All pipeline and buffer setup can be easily customized inside setup.h.
- Written in C++20 (I love designated initializers) using Vulkan RAII and GLFW for window output (resize is disabled).
## Usage
- You can add buffers, storage images, or even new compute shader passes simply by adding the relevant information to a few vectors. The main program will automatically set them up during execution.
- do note that all the buffer and image you setup will be available to "all shader pass" (since they all share the same descriptor)
- all the setup only take place in `setup.h`
### Adding image
- **currently only support storage image (sampled image is planned for future)**
1. fill in the `ImageInfo` struct
```cpp=
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
```
2. add them into the `imageInfos` vector inside `setup.h` and you should be good to go!
    - do note that `binding = 0` is reserve for the output image (for real time output)... so just don't touch the first element in the `imageInfos` vector
```cpp=
std::vector<ImageInfo> imageInfos = {
    {.name = "outputImage", .binding = 0, .extent = glm::vec2(width, height), .format = vk::Format::eR8G8B8A8Unorm,        .usage = storageImageUsageFlags, .memoryProperty = storageImageMemoryPropertyFlags, .descriptorType = storageImageDescriptorType},
    {.name = "image",       .binding = 1, .extent = glm::vec2(width, height), .format = vk::Format::eR32G32B32A32Sfloat,   .usage = storageImageUsageFlags, .memoryProperty = storageImageMemoryPropertyFlags, .descriptorType = storageImageDescriptorType},
};
```
### Adding Buffer
1. fill in the `BufferInfo` struct
```cpp=
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
```
2. add them into the `bufferInfos` vector inside `setup.h` and you should be good to go!
```cpp=
std::vector<BufferInfo> bufferInfos = {
    {.name = "parameters",  .binding = 2, .size = sizeof(parameters),   .usage = uniformBufferUsageFlags,   .memoryProperty = uniformBufferMemoryPropertyFlags,     .descriptorType = uniformBufferDescriptorType,  .data = &parameters},
    {.name = "intArray",    .binding = 3, .size = sizeof(numA),         .usage = storageBufferUsageFlags,   .memoryProperty = storageBufferMemoryPropertyFlags,     .descriptorType = storageBufferDescriptorType,  .data = &numA},
};
```
### Adding shader pass
very similar to how we setup buffer and image!
1. fill in the `ShaderInfo` struct
```cpp=
struct ShaderInfo
{
    std::string name;
    std::string filePath;
    std::string entryPoint;
    glm::vec3 globalDimension;
    glm::vec3 localDimension;
};
```
2. add them into the `shaderInfos` vector inside `setup.h` and you should be good to go!
    - the shader will be execute in the exact order inside the `shaderInfos` vector!
```cpp=
const std::vector<ShaderInfo> shaderInfos = {
    {.name = "shadertoyDefault",   .filePath = "../../shader/shadertoyDefault.comp",   .entryPoint = "main", .globalDimension = glm::vec3(width, height, 1), .localDimension = localDimension},
    {.name = "shadertoyTest",   .filePath = "../../shader/shadertoyTest.comp",            .entryPoint = "main", .globalDimension = glm::vec3(width, height, 1), .localDimension = localDimension},
    {.name = "shadertoyAccretion", .filePath = "../../shader/shadertoyAccretion.comp", .entryPoint = "main", .globalDimension = glm::vec3(width, height, 1), .localDimension = localDimension},
    {.name = "shadertoyTunnel", .filePath = "../../shader/shadertoyTunnel.comp", .entryPoint = "main", .globalDimension = glm::vec3(width, height, 1), .localDimension = localDimension},
};
```
### Data initialize and update
you can initialize the data in buffer by modifying the `initializeDeviceData()`
- this function will be called at the start of the main program
```cpp=
void initializeDeviceData()
{
    parameters.screenDimension = glm::vec3(width, height, 1);
    parameters.time = 0;
    start = std::chrono::system_clock::now();
}

```
similarly, you can also update the buffer data each frame by modifying the `updateDeviceData`
```cpp=
void updateDeviceData(const std::vector<Buffer> &buffers, const std::vector<Image> &images)
{
    end = std::chrono::system_clock::now();
    parameters.time = std::chrono::duration<float>(end - start).count();
    updateBuffer(buffers, bufferInfos, "parameters");
}
```
## Gallery
![未命名](https://hackmd.io/_uploads/S1mt10kSlx.jpg)
- left and right side are produce with different shader pass
    - left is produced by the `shadertoyAccretion` shader from [shadertoy](https://www.shadertoy.com/view/WcKXDV)
    - right is produced by the `shadertoyTunnel` shader from [shadertoy](https://www.shadertoy.com/view/WfcGWj)
## TODO
- imGUI support
- keyborad input
