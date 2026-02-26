#include "vulkan_imgui.hpp"

VulkanImgui::VulkanImgui( VkQueue graphicsFam, uint32_t index )
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // IF using Docking Branch

  ImGui_ImplSDL2_InitForVulkan( window );

  auto queue = findQueueFamilies( m_physicalDevice );
  ImGui_ImplVulkan_InitInfo info;
  info.DescriptorPoolSize = 0;
  info.DescriptorPool = m_descriptorPool;
  info.PipelineInfoMain.RenderPass = nullptr;
  info.PipelineInfoForViewports.RenderPass = nullptr;
  info.UseDynamicRendering = true;
  info.Device = m_device;
  info.PhysicalDevice = m_physicalDevice;
  info.ImageCount = 2;
  info.MinImageCount = 2;
  info.Queue = m_graphicsQueue;
  info.PipelineCache = VK_NULL_HANDLE;
  info.QueueFamily = queue.graphicsFamily.value();
  info.Instance = m_instance;
  info.Allocator = nullptr;
  info.CheckVkResultFn = nullptr;
  info.MinAllocationSize = 1024 * 1024;
  info.ApiVersion = VK_API_VERSION_1_4;
  ImGui_ImplVulkan_Init( &info );
}

VulkanImgui::~VulkanImgui()
{
}

void VulkanImgui::render()
{
}

void VulkanImgui::present()
{
}
