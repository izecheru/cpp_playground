set(Target_sources
"include/vulkan_abstraction/vulkan_device.hpp"
"include/vulkan_abstraction/vulkan_swapchain.hpp"
"include/vulkan_abstraction/vulkan_renderer.hpp"
"include/vulkan_abstraction/vulkan_imgui_renderer.hpp"

"src/vulkan_device.cpp"
"src/vulkan_renderer.cpp"
"src/vulkan_swapchain.cpp"
"src/vulkan_imgui_renderer.cpp"
)
add_library(vulkan_abstraction STATIC "${Target_sources}"  "src/vulkan_renderer.cpp")

target_include_directories(vulkan_abstraction PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(vulkan_abstraction PRIVATE Vulkan::Vulkan SDL2::SDL2 spdlog::spdlog imgui)
