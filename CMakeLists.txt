cmake_minimum_required(VERSION 3.10.0)
project(compute VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 20)

if (DEFINED VULKAN_SDK_PATH)
  set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_PATH}/Include") # 1.1 Make sure this include path is correct
  set(Vulkan_LIBRARIES "${VULKAN_SDK_PATH}/Lib") # 1.2 Make sure lib path is correct
  set(Vulkan_FOUND "True")
else()
  find_package(Vulkan REQUIRED COMPONENTS shaderc_combined) # throws error if could not find Vulkan
  message(STATUS "Found Vulkan: $ENV{VULKAN_SDK}")
endif()

find_package(GLFW3 REQUIRED)
message(STATUS "Found GLFW3 in ${GLFW3_INCLUDE_DIR}")

add_executable(compute 
    common.h
    setup.h
    main.cpp
)

target_link_directories(compute PRIVATE external)

target_link_libraries(compute ${GLFW3_LIBRARY} Vulkan::Vulkan Vulkan::shaderc_combined)

target_include_directories(compute PRIVATE ${GLFW3_INCLUDE_DIR})
