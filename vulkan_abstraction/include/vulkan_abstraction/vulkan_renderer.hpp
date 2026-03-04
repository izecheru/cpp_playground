#pragma once
#include <memory>
#include <vulkan/vulkan.h>

struct SDL_Window;
class VulkanImguiRenderer;
class VulkanDevice;
class VulkanSwapchain;

class VulkanRenderer
{
public:
  explicit VulkanRenderer( SDL_Window* wnd );
  ~VulkanRenderer() = default;

  void beginRendering();
  void endRendering();
  void renderImgui( VkCommandBuffer& cmd );
  void presentFrame();

private:
  std::shared_ptr<VulkanDevice> m_device;
  std::shared_ptr<VulkanSwapchain> m_swapchain;
  std::shared_ptr<VulkanImguiRenderer> m_imguiRenderer;

  VkPipeline m_graphicsPipeline;
  VkPipelineLayout m_graphicsPipelineLayout;
};