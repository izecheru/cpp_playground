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

  void render();
  void present( VkCommandBuffer& buffer );

private:
  void initImgui( SDL_Window* wnd, VulkanDevice* device, VulkanSwapchain* swapchain );

  void begin();
  void end();

private:
  VkDescriptorPool m_descriptorPool;
  VulkanDevice* m_device;
};