#include "vulkan_abstraction/vulkan_imgui_renderer.hpp"
#include <stdexcept>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"
#include "vulkan_abstraction/vulkan_device.hpp"
#include "vulkan_abstraction/vulkan_swapchain.hpp"

VulkanImguiRenderer::VulkanImguiRenderer( SDL_Window* wnd, VulkanDevice* device, VulkanSwapchain* swapchain )
    : m_device{ device }
{
  initImgui( wnd, device, swapchain );
}

VulkanImguiRenderer::~VulkanImguiRenderer()
{
  vkDeviceWaitIdle( m_device->getLogicalDevice() );
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  vkDestroyDescriptorPool( m_device->getLogicalDevice(), m_descriptorPool, nullptr );
}

void VulkanImguiRenderer::begin()
{
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
}

void VulkanImguiRenderer::end()
{
  ImGui::Render();
}

void VulkanImguiRenderer::render()
{
  begin();

  // TODO iterate through the windows and draw them here
  auto mouse = ImGui::GetMousePos();
  ImGui::Begin( "test" );
  auto window = ImGui::GetWindowPos();
  ImGui::Text( "Mouse pos %.2f %.2f", mouse.x, mouse.y );
  ImGui::Text( "Window pos %.2f %.2f", window.x, window.y );
  ImGui::End();
  end();
}

void VulkanImguiRenderer::present( VkCommandBuffer& buffer )
{
  auto drawData = ImGui::GetDrawData();
  const bool isMinimized = ( drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f );

  if ( !isMinimized )
    ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), buffer, NULL );

  if ( ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
  {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
  }
}

void VulkanImguiRenderer::initImgui( SDL_Window* wnd, VulkanDevice* device, VulkanSwapchain* swapchain )
{
  VkDescriptorPoolSize pool_sizes[] = {
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
  };
  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.maxSets = 0;
  for ( VkDescriptorPoolSize& pool_size : pool_sizes )
    poolInfo.maxSets += pool_size.descriptorCount;
  poolInfo.poolSizeCount = (uint32_t)IM_COUNTOF( pool_sizes );
  poolInfo.pPoolSizes = pool_sizes;

  if ( vkCreateDescriptorPool( device->getLogicalDevice(), &poolInfo, nullptr, &m_descriptorPool ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create imgui descriptor pool!" );
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

  // TODO get this from swapchain
  init_info.MinImageCount = 2;
  init_info.ImageCount = 3;

  init_info.UseDynamicRendering = true;
  init_info.PipelineInfoMain.PipelineRenderingCreateInfo = { .sType =
                                                               VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };

  init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
  init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchain->getSwapchainFormat();

  ImGui_ImplVulkan_Init( &init_info );
}
