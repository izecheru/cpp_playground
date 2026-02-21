set(Target_sources
"include/vulkan_device/vulkan_device.hpp"
"include/vulkan_swapchain/vulkan_swapchain.hpp"
"include/vulkan_pipeline/vulkan_pipeline.hpp"
"src/vulkan_device.cpp"
"src/vulkan_swapchain.cpp"
 "src/vulkan_pipeline.cpp")
add_library(vulkan_abstraction STATIC "${Target_sources}" )

target_include_directories(vulkan_abstraction PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(vulkan_abstraction PRIVATE Vulkan::Vulkan glfw spdlog::spdlog)
