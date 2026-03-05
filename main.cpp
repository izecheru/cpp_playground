#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <iostream>
#include "vulkan_abstraction/vulkan_device.hpp"
#include "vulkan_abstraction/vulkan_imgui_renderer.hpp"
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

    auto device = std::make_shared<VulkanDevice>( window );
    auto swapchain = std::make_shared<VulkanSwapchain>( device.get(), window );
    auto imgui = std::make_shared<VulkanImguiRenderer>( window, device.get(), swapchain.get() );

    static bool running = true;
    while ( running )
    {
      SDL_Event e;
      static bool rendering = true;
      while ( SDL_PollEvent( &e ) )
      {
        ImGui_ImplSDL2_ProcessEvent( &e );
        switch ( e.type )
        {
        case SDL_QUIT:
          running = false;
          break;
        }

        swapchain->beginRendering();
        imgui->render( swapchain->getCurrentCommandBuffer() );
        swapchain->endRendering();

        swapchain->presentFrame();
      }
    }
  }
  catch ( std::exception& e )
  {
    std::cout << "exception: " << e.what();
  }
  return 0;
}