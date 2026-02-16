cmake_minimum_required(VERSION 3.26)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE
        "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

project (enttTest)
set(CMAKE_CXX_STANDARD 20)

add_executable (enttTest "enttTest.cpp" "enttTest.h" "vulkan_backend.hpp")
find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(enttTest PRIVATE Vulkan::Vulkan glfw)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET enttTest PROPERTY CXX_STANDARD 20)
endif()

add_subdirectory(dependencies/lua)
