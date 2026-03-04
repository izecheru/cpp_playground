#pragma once

struct SDL_Window;
class VulkanDevice;
class VulkanSwapchain;

class VulkanImguiRenderer
{
public:
  explicit VulkanImguiRenderer( SDL_Window* wnd, VulkanDevice* device, VulkanSwapchain* swapchain );
  ~VulkanImguiRenderer() = default;

  void initImgui( SDL_Window* wnd, VulkanDevice* device, VulkanSwapchain* swapchain );

private:
  VkDescriptorPool m_descriptorPool;
};