#include "vulkan_abstraction/vulkan_imgui_renderer.hpp"
#include <stdexcept>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"
#include "vulkan_abstraction/vulkan_device.hpp"
#include "vulkan_abstraction/vulkan_swapchain.hpp"

VulkanImguiRenderer::VulkanImguiRenderer( SDL_Window* wnd, VulkanDevice* device, VulkanSwapchain* swapchain )
{
  initImgui( wnd, device, swapchain );
}

void VulkanImguiRenderer::initImgui( SDL_Window* wnd, VulkanDevice* device, VulkanSwapchain* swapchain )
{
  VkDescriptorPoolSize pool_sizes[] = {
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
  };
  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 0;
  for ( VkDescriptorPoolSize& pool_size : pool_sizes )
    pool_info.maxSets += pool_size.descriptorCount;
  pool_info.poolSizeCount = (uint32_t)IM_COUNTOF( pool_sizes );
  pool_info.pPoolSizes = pool_sizes;

  if ( vkCreateDescriptorPool( device->getLogicalDevice(), &pool_info, nullptr, &m_descriptorPool ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create descriptor pool!" );
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  ImGui_ImplSDL2_InitForVulkan( wnd );

  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = device->getInstance();
  init_info.PhysicalDevice = device->getPhysicalDevice();
  init_info.Device = device->getLogicalDevice();
  init_info.QueueFamily = device->getGraphicsQueue().familyIndex;
  init_info.Queue = device->getGraphicsQueue().handle;
  init_info.DescriptorPool = m_descriptorPool;
  init_info.MinImageCount = 2;
  init_info.ImageCount = 2;
  init_info.UseDynamicRendering = true;
  init_info.PipelineInfoMain.PipelineRenderingCreateInfo = { .sType =
                                                               VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };

  init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
  init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats =
    &swapchain->getSwapchainImageFormat();
  // init_info.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = findDepthFormat();

  ImGui_ImplVulkan_Init( &init_info );
}
