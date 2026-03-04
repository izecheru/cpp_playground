#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <iostream>
#include "vulkan_abstraction/vulkan_device.hpp"
#include "vulkan_abstraction/vulkan_swapchain.hpp"

int main()
{
  try
  {
    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) != 0 )
    {
      throw std::runtime_error( "SDL_Init failed" );
    }

    if ( SDL_Vulkan_LoadLibrary( nullptr ) != 0 )
    {
      throw std::runtime_error( "could not load lib vulkan" );
    }

    auto window =
      SDL_CreateWindow( "kogayonon", 100, 100, 800, 800, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN );

    VulkanDevice device{ window };
    VulkanSwapchain swapchain{ &device, window };
  }
  catch ( std::exception& e )
  {
    std::cout << "something went wrong: exception-> " << e.what();
  }
  return 0;
}