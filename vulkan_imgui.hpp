#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.h>

class VulkanImgui
{
public:
  VulkanImgui();
  ~VulkanImgui();

  void render();
  void present();

private:
};