#include "vulkan_abstraction/vulkan_renderer.hpp"
#include "imgui_impl_vulkan.h"
#include "vulkan_abstraction/vulkan_device.hpp"
#include "vulkan_abstraction/vulkan_imgui_renderer.hpp"
#include "vulkan_abstraction/vulkan_swapchain.hpp"

VulkanRenderer::VulkanRenderer( SDL_Window* wnd )
{
  m_device = std::make_shared<VulkanDevice>( wnd );
  m_swapchain = std::make_shared<VulkanSwapchain>( m_device.get(), wnd );
  m_imguiRenderer = std::make_shared<VulkanImguiRenderer>( wnd, m_device.get(), m_swapchain.get() );
}

void VulkanRenderer::beginRendering()
{
  //auto currentFrame = m_swapchain->getCurrentSwapchainImage();

  //vkWaitForFences( m_device->getLogicalDevice(), 1, &currentFrame.inFlight, VK_TRUE, UINT64_MAX );

  //m_swapchain->getNextImageIndex();

  //vkResetFences( m_device->getLogicalDevice(), 1, &currentFrame.inFlight );
}

void VulkanRenderer::endRendering()
{
}

void VulkanRenderer::renderImgui( VkCommandBuffer& cmd )
{
  ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), cmd );
}

void VulkanRenderer::presentFrame()
{
}
