#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <iostream>
#include <vector>
#include "vulkan_backend.hpp"
#pragma clang diagnostic pop
#include <vulkan/vulkan.hpp>

#ifdef _WIN32
// #pragma comment( linker, "/subsystem:windows" )
#define VK_USE_PLATFORM_WIN32_KHR
#define PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif

#define APP_NAME "hello-triangle"
#define ARRAY_SIZE( a ) ( sizeof( a ) / sizeof( a[0] ) )
#ifndef UINT32_MAX
#define UINT32_MAX 0xffffffff
#endif

int SDL_main( int argc, char* argv[] )
{
  if ( SDL_Init( SDL_INIT_VIDEO ) != 0 )
  {
    std::cout << "failed to init sdl";
    return 0;
  }

  if ( SDL_Vulkan_LoadLibrary( nullptr ) == -1 )
  {
    std::cout << "failed to load lib";
    return 0;
  }

  SDL_Window* m_window = SDL_CreateWindow(
    APP_NAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 360, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN );
  VulkanBase vulkan( m_window );

  bool running = true;
  while ( running )
  {
    SDL_Event windowEvent;
    while ( SDL_PollEvent( &windowEvent ) )
    {
      switch ( windowEvent.type )
      {
      case SDL_QUIT:
        running = false;
      case SDL_KEYDOWN:
        std::cout << windowEvent.key.keysym.scancode << '\n';
      }
    }
  }

  return 0;
}
