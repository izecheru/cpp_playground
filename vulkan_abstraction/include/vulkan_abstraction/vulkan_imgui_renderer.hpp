#pragma once
#include <vulkan/vulkan.h>

struct SDL_Window;
class VulkanDevice;
class VulkanSwapchain;

class VulkanImguiRenderer
{
public:
  explicit VulkanImguiRenderer( SDL_Window* wnd, VulkanDevice* device, VulkanSwapchain* swapchain );
  ~VulkanImguiRenderer();

  void render( VkCommandBuffer& buffer );
  void initImgui( SDL_Window* wnd, VulkanDevice* device, VulkanSwapchain* swapchain );

private:
  void begin();
  void end();

private:
  VkDescriptorPool m_descriptorPool;
  VulkanDevice* m_device;
};