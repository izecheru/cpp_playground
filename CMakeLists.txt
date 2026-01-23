cmake_minimum_required(VERSION 3.26)


if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE
        "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

project (enttTest)
set(CMAKE_CXX_STANDARD 20)

add_executable (enttTest "enttTest.cpp" "enttTest.h" "entity.hpp" "registry.hpp" "event_dispatcher.hpp")
find_package(EnTT CONFIG REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
target_link_libraries(enttTest PRIVATE EnTT::EnTT lua)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET enttTest PROPERTY CXX_STANDARD 20)
endif()

add_subdirectory(dependencies/lua)

set_property(TARGET enttTest PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
