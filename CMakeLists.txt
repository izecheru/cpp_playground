cmake_minimum_required(VERSION 3.26)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE
        "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

project (enttTest)
set(CMAKE_CXX_STANDARD 20)

#add_executable (enttTest "enttTest.cpp" "enttTest.h" "entity.hpp" "registry.hpp" "event_dispatcher.hpp")
add_executable (enttTest "enttTest.cpp" "enttTest.h" "vulkan_backend.hpp")
target_link_libraries(enttTest PRIVATE Vulkan::Vulkan  SDL2::SDL2 SDL2::SDL2main)
find_package(Vulkan REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
#add_subdirectory("dependencies/glad")
#find_package(EnTT CONFIG REQUIRED)
#find_package(sol2 CONFIG REQUIRED)
#find_package(glm CONFIG REQUIRED)
#find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
#target_link_libraries(enttTest PRIVATE EnTT::EnTT lua)
#target_include_directories(enttTest PRIVATE ${TINYGLTF_INCLUDE_DIRS})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET enttTest PROPERTY CXX_STANDARD 20)
endif()

add_subdirectory(dependencies/lua)
