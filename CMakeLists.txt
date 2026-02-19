cmake_minimum_required(VERSION 3.26)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE
        "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

project (enttTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(/utf-8)
add_executable (enttTest "main.cpp")
find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET enttTest PROPERTY CXX_STANDARD 20)
endif()

add_subdirectory("dependencies/lua")
add_subdirectory("vulkan_abstraction")
target_link_libraries(enttTest PRIVATE Vulkan::Vulkan glfw spdlog::spdlog vulkan_abstraction)
