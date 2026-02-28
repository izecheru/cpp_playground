#define STB_IMAGE_IMPLEMENTATION
#include "vulkan_backend.hpp"
#include <chrono>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <set>
#include <spdlog/spdlog.h>
#include "stb_image.h"

void VulkanBase::initVulkan()
{
  initWindow();

  createInstance();
  setupDebug();

  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();

  createSwapChain();
  createImageViews();

  createCommandPool();
  createCommandBuffer();

  createDepthResources();
  createViewport();
  createUniformBuffers();
  createTextureImage();
  createTextureImageView();
  createTextureSamplers();
  createDescriptorSetLayout();
  createDescriptorPool();
  createDescriptorSets();

  createGraphicsPipeline();

  createVertexBuffer();
  createIndexBuffer();

  createSyncObj();
  initImgui();
}

void VulkanBase::createUniformBuffers()
{
  VkDeviceSize bufferSize = sizeof( UniformBufferoObject );
  m_uniformBuffers.resize( MAX_FRAMES_IN_FLIGHT );
  m_uniformBuffersMemory.resize( MAX_FRAMES_IN_FLIGHT );
  m_uniformBuffersMapped.resize( MAX_FRAMES_IN_FLIGHT );

  for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
  {
    createBuffer( bufferSize,
                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  m_uniformBuffers[i],
                  m_uniformBuffersMemory[i] );

    vkMapMemory( m_device, m_uniformBuffersMemory[i], 0, bufferSize, 0, &m_uniformBuffersMapped[i] );
  }
}

void VulkanBase::recordCommandBuffer( VkCommandBuffer& cmd, uint32_t imageIndex )
{
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  vkBeginCommandBuffer( cmd, &beginInfo );

  VkImageMemoryBarrier textureToColor = {};
  textureToColor.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  textureToColor.srcAccessMask = 0;
  textureToColor.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  textureToColor.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  textureToColor.subresourceRange.levelCount = 1;
  textureToColor.subresourceRange.layerCount = 1;

  textureToColor.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  textureToColor.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  textureToColor.image = m_viewport.image;

  VkImageMemoryBarrier depthBarrier = {};
  depthBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  depthBarrier.srcAccessMask = 0;
  depthBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  depthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  depthBarrier.subresourceRange.levelCount = 1;
  depthBarrier.subresourceRange.layerCount = 1;

  depthBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
  depthBarrier.image = m_depthImage;

  vkCmdPipelineBarrier( cmd,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &textureToColor );

  vkCmdPipelineBarrier( cmd,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &depthBarrier );

  VkRenderingAttachmentInfo depthAttachment{ .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                                             .imageView = m_depthView,
                                             .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                             .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                             .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                                             .clearValue = { .depthStencil = { 1.f, 0 } } };

  VkRenderingAttachmentInfo colorAttachment{ .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                                             //.imageView = m_swapChainImageViews[imageIndex],
                                             .imageView = m_viewport.imageView,
                                             .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                             .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                             .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                                             .clearValue = { { 0.f, 0.f, 0.f, 1.f } } };

  VkRenderingInfo renderingInfo = {};
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderingInfo.renderArea = { { 0, 0 }, m_swapChainExtent };
  renderingInfo.layerCount = 1;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachments = &colorAttachment;
  renderingInfo.pDepthAttachment = &depthAttachment;

  vkCmdBeginRendering( cmd, &renderingInfo );

  vkCmdBindPipeline( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline );

  VkViewport viewport{ 0, 0, (float)m_swapChainExtent.width, (float)m_swapChainExtent.height, 0.0f, 1.0f };
  vkCmdSetViewport( cmd, 0, 1, &viewport );

  VkRect2D scissor{ { 0, 0 }, m_swapChainExtent };
  vkCmdSetScissor( cmd, 0, 1, &scissor );

  VkBuffer vertexBuffers[] = { m_vertexBuffer };
  VkDeviceSize offsets[] = { 0 };

  vkCmdBindVertexBuffers( cmd, 0, 1, vertexBuffers, offsets );
  vkCmdBindIndexBuffer( cmd, m_indicesBuffer, 0, VK_INDEX_TYPE_UINT16 );
  vkCmdBindDescriptorSets(
    cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[imageIndex], 0, nullptr );

  vkCmdDrawIndexed( cmd, static_cast<uint32_t>( indices.size() ), 1, 0, 0, 0 );

  vkCmdEndRendering( cmd );

  VkImageMemoryBarrier textureToShader = {};
  textureToShader.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  textureToShader.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  textureToShader.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  textureToShader.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  textureToShader.subresourceRange.baseMipLevel = 0;
  textureToShader.subresourceRange.levelCount = 1;
  textureToShader.subresourceRange.baseArrayLayer = 0;
  textureToShader.subresourceRange.layerCount = 1;

  textureToShader.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  textureToShader.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  textureToShader.image = m_viewport.image;

  // Fix the pipeline stages too!
  vkCmdPipelineBarrier( cmd,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &textureToShader );
  vkCmdPipelineBarrier( cmd,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &textureToShader );

  VkImageMemoryBarrier swapchainToColor = {};
  swapchainToColor.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  swapchainToColor.srcAccessMask = 0;
  swapchainToColor.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  swapchainToColor.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  swapchainToColor.subresourceRange.baseMipLevel = 0;
  swapchainToColor.subresourceRange.levelCount = 1;
  swapchainToColor.subresourceRange.baseArrayLayer = 0;
  swapchainToColor.subresourceRange.layerCount = 1;
  swapchainToColor.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  swapchainToColor.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  swapchainToColor.image = m_swapChainImages[imageIndex];

  vkCmdPipelineBarrier( cmd,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &swapchainToColor );

  VkRenderingAttachmentInfo imguiColorAttachment{};
  imguiColorAttachment.imageView = m_swapChainImageViews[imageIndex];
  imguiColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkRenderingInfo imguiRenderingInfo{};
  imguiRenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  imguiRenderingInfo.renderArea = { { 0, 0 }, m_swapChainExtent };
  imguiRenderingInfo.layerCount = 1;
  imguiRenderingInfo.colorAttachmentCount = 1;
  imguiRenderingInfo.pColorAttachments = &imguiColorAttachment;
  imguiRenderingInfo.pDepthAttachment = nullptr;

  vkCmdBeginRendering( cmd, &imguiRenderingInfo );
  imguiBegin();
  ImGui::Begin( "Viewport" );
  static auto viewportDescriptorSet =
    ImGui_ImplVulkan_AddTexture( m_textureSampler, m_viewport.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
  ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
  ImGui::Image( viewportDescriptorSet, ImVec2{ viewportPanelSize.x, viewportPanelSize.y } );

  ImGui::End();
  ImGui::Begin( "Test2" );
  ImGui::Text( "Test window 2" );
  ImGui::End();

  ImGui::Render();

  ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), cmd );
  vkCmdEndRendering( cmd );
  VkImageMemoryBarrier swapchainToPresent{};
  swapchainToPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  swapchainToPresent.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  swapchainToPresent.dstAccessMask = 0;
  swapchainToPresent.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  swapchainToPresent.subresourceRange.baseMipLevel = 0;
  swapchainToPresent.subresourceRange.levelCount = 1;
  swapchainToPresent.subresourceRange.baseArrayLayer = 0;
  swapchainToPresent.subresourceRange.layerCount = 1;

  swapchainToPresent.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  swapchainToPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  swapchainToPresent.image = m_swapChainImages[imageIndex];

  vkCmdPipelineBarrier( cmd,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // src: after color attachment
                        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          // dst: before present
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &swapchainToPresent );
  vkCmdPipelineBarrier( cmd,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &swapchainToPresent );

  vkEndCommandBuffer( cmd );
}

void VulkanBase::createCommandBuffer()
{
  m_commandBuffers.resize( MAX_FRAMES_IN_FLIGHT );
  for ( auto i = 0u; i < MAX_FRAMES_IN_FLIGHT; i++ )
  {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if ( vkAllocateCommandBuffers( m_device, &allocInfo, &m_commandBuffers.at( i ) ) != VK_SUCCESS )
    {
      throw std::runtime_error( "failed to allocate command buffers!" );
    }
  }
}

void VulkanBase::createCommandPool()
{
  QueueFamilyIndices queueFamilyIndices = findQueueFamilies( m_physicalDevice );

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
  if ( vkCreateCommandPool( m_device, &poolInfo, nullptr, &m_commandPool ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create command pool!" );
  }
}

static auto readFile( const std::string& filePath ) -> std::vector<char>
{
  std::ifstream file( filePath, std::ios::ate | std::ios::binary );

  if ( !file.is_open() )
  {
    throw std::runtime_error( "failed to open shader file!\n" );
  }
  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer( fileSize );
  file.seekg( 0 );
  file.read( buffer.data(), fileSize );
  file.close();

  return buffer;
}

void VulkanBase::copyBuffer( VkBuffer src, VkBuffer dst, VkDeviceSize size )
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferCopy copyRegion{};
  copyRegion.size = size;
  vkCmdCopyBuffer( commandBuffer, src, dst, 1, &copyRegion );

  endSingleTimeCommands( commandBuffer );
}

void VulkanBase::createViewport()
{
  createImage( m_swapChainExtent.width,
               m_swapChainExtent.height,
               m_swapChainImageFormat,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
               m_viewport.image,
               m_viewport.memory );

  m_viewport.imageView = createImageView( m_viewport.image, m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT );
}

void VulkanBase::setupDockspace( ImGuiViewport* viewport )
{
  static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_AutoHideTabBar;
  if ( ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable )
  {
    const auto dockspaceId = ImGui::GetID( "MyDockspace" );
    ImGui::DockSpace( dockspaceId, ImVec2{ 0, 0 }, dockspaceFlags );
    if ( !std::filesystem::exists( ImGui::GetIO().IniFilename ) )
    {
      // Clear previous layout
      ImGui::DockBuilderRemoveNode( dockspaceId );
      ImGui::DockBuilderAddNode( dockspaceId, ImGuiDockNodeFlags_DockSpace );
      ImGui::DockBuilderSetNodeSize( dockspaceId, viewport->Size );

      auto centerNodeId = dockspaceId;
      auto leftNodeId = ImGui::DockBuilderSplitNode( centerNodeId, ImGuiDir_Left, 0.2f, nullptr, &centerNodeId );
      auto rightNodeId = ImGui::DockBuilderSplitNode( centerNodeId, ImGuiDir_Right, 0.2f, nullptr, &centerNodeId );
      auto bottomCenterNodeId =
        ImGui::DockBuilderSplitNode( centerNodeId, ImGuiDir_Down, 0.3f, nullptr, &centerNodeId );

      auto upplerLeftNodeId = ImGui::DockBuilderSplitNode( leftNodeId, ImGuiDir_Up, 0.4f, nullptr, &leftNodeId );
      auto lowerLeftNodeId = ImGui::DockBuilderSplitNode( leftNodeId, ImGuiDir_Up, 0.3f, nullptr, &leftNodeId );

      ImGui::DockBuilderFinish( dockspaceId );
    }
  }
}

void VulkanBase::imguiBegin()
{
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                         ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  static ImGuiViewport* viewport = ImGui::GetMainViewport();
  static float menu_bar_height = ImGui::GetFrameHeight();

  ImGui::SetNextWindowPos( ImVec2( viewport->Pos.x, viewport->Pos.y + menu_bar_height ) );
  ImGui::SetNextWindowSize( ImVec2( viewport->Size.x, viewport->Size.y - menu_bar_height ) );
  ImGui::SetNextWindowViewport( viewport->ID );

  ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );
  ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );

  ImGui::Begin( "Main", nullptr, window_flags );
  setupDockspace( viewport );
  ImGui::PopStyleVar( 2 );
  ImGui::End();
}

void VulkanBase::initImgui()
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

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // IF using Docking Branch

  ImGui_ImplSDL2_InitForVulkan( window );

  auto queue = findQueueFamilies( m_physicalDevice );
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = m_instance;
  init_info.PhysicalDevice = m_physicalDevice;
  init_info.Device = m_device;
  init_info.QueueFamily = queue.graphicsFamily.value();
  init_info.Queue = m_graphicsQueue;
  init_info.DescriptorPool = m_descriptorPool;
  init_info.MinImageCount = 2;
  init_info.ImageCount = 2;
  init_info.UseDynamicRendering = true;
  init_info.PipelineInfoMain.PipelineRenderingCreateInfo = { .sType =
                                                               VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };

  init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
  init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_swapChainImageFormat;

  init_info.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = findDepthFormat();

  ImGui_ImplVulkan_Init( &init_info );
}

void VulkanBase::recreateSwapchain()
{
  int width = 0, height = 0;
  SDL_GetWindowSize( window, &width, &height );
  if ( width == 0 || height == 0 )
    return;

  vkDeviceWaitIdle( m_device );
  cleanSwapchain();

  createSwapChain();
  createImageViews();
  createDepthResources();
  createTextureImage();
  createTextureImageView();
  createTextureSamplers();

  createUniformBuffers();
  createDescriptorSetLayout();
  createDescriptorPool();
  createDescriptorSets();

  createCommandBuffer();
}

void VulkanBase::createVertexBuffer()
{
  VkDeviceSize bufferSize = sizeof( vertices[0] ) * vertices.size();
  createBuffer( bufferSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_stageBuffer,
                m_stageBufferMemory );

  void* data;
  vkMapMemory( m_device, m_stageBufferMemory, 0, bufferSize, 0, &data );
  memcpy( data, vertices.data(), (size_t)bufferSize );
  vkUnmapMemory( m_device, m_stageBufferMemory );

  createBuffer( bufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_vertexBuffer,
                m_vertexBufferMemory );

  copyBuffer( m_stageBuffer, m_vertexBuffer, bufferSize );

  vkDestroyBuffer( m_device, m_stageBuffer, nullptr );
  vkFreeMemory( m_device, m_stageBufferMemory, nullptr );
}

void VulkanBase::createIndexBuffer()
{
  VkDeviceSize bufferSize = sizeof( indices[0] ) * indices.size();

  createBuffer( bufferSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_stageBuffer,
                m_stageBufferMemory );

  void* data;
  vkMapMemory( m_device, m_stageBufferMemory, 0, bufferSize, 0, &data );
  memcpy( data, indices.data(), (size_t)bufferSize );
  vkUnmapMemory( m_device, m_stageBufferMemory );

  createBuffer( bufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_indicesBuffer,
                m_indicesBufferMemory );

  copyBuffer( m_stageBuffer, m_indicesBuffer, bufferSize );

  vkDestroyBuffer( m_device, m_stageBuffer, nullptr );
  vkFreeMemory( m_device, m_stageBufferMemory, nullptr );
}

void VulkanBase::updateUniformBuffer( uint32_t imageIndex )
{
  static auto startTime = std::chrono::high_resolution_clock::now();

  auto currentTime = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>( currentTime - startTime ).count();

  UniformBufferoObject ubo{};
  ubo.model = glm::rotate( glm::mat4( 1.0f ), time * glm::radians( 90.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );
  ubo.view = glm::lookAt( glm::vec3( 3.0f, 3.0f, 3.0f ), glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );
  ubo.proj =
    glm::perspective( glm::radians( 65.0f ), m_swapChainExtent.width / (float)m_swapChainExtent.height, 0.1f, 10.0f );
  ubo.proj[1][1] *= -1;

  memcpy( m_uniformBuffersMapped[imageIndex], &ubo, sizeof( ubo ) );
}

void VulkanBase::createDescriptorSetLayout()
{
  VkDescriptorSetLayoutBinding uboLayoutBinding{};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.pImmutableSamplers = nullptr;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutBinding samplerLayoutBinding{};
  samplerLayoutBinding.binding = 1;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>( bindings.size() );
  layoutInfo.pBindings = bindings.data();

  if ( vkCreateDescriptorSetLayout( m_device, &layoutInfo, nullptr, &m_descriptorSetLayout ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create descriptor set layout!" );
  }
}

void VulkanBase::createDescriptorPool()
{
  std::array<VkDescriptorPoolSize, 2> poolSizes{
    VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT },
    VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAMES_IN_FLIGHT * 2 } };

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.poolSizeCount = static_cast<uint32_t>( poolSizes.size() );
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = static_cast<uint32_t>( MAX_FRAMES_IN_FLIGHT ) * 2;

  if ( vkCreateDescriptorPool( m_device, &poolInfo, nullptr, &m_descriptorPool ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create descriptor pool!" );
  }
}

void VulkanBase::copyBufferToImage( VkBuffer buffer, VkImage image, uint32_t width, uint32_t height )
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = { 0, 0, 0 };
  region.imageExtent = { width, height, 1 };

  vkCmdCopyBufferToImage( commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );

  endSingleTimeCommands( commandBuffer );
}

void VulkanBase::createTextureImageView()
{
  m_textureView = createImageView( m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT );
}

auto VulkanBase::beginSingleTimeCommands() -> VkCommandBuffer
{
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = m_commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers( m_device, &allocInfo, &commandBuffer );

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer( commandBuffer, &beginInfo );

  return commandBuffer;
}

void VulkanBase::endSingleTimeCommands( VkCommandBuffer commandBuffer )
{
  vkEndCommandBuffer( commandBuffer );

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit( m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
  vkQueueWaitIdle( m_graphicsQueue );

  vkFreeCommandBuffers( m_device, m_commandPool, 1, &commandBuffer );
}

void VulkanBase::transitionImageLayout( VkImage image,
                                        VkFormat format,
                                        VkImageLayout oldLayout,
                                        VkImageLayout newLayout )
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else
  {
    throw std::invalid_argument( "unsupported layout transition!" );
  }

  vkCmdPipelineBarrier( commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier );

  endSingleTimeCommands( commandBuffer );
}

void VulkanBase::createDescriptorSets()
{
  std::vector<VkDescriptorSetLayout> layouts( MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout );
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = m_descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>( MAX_FRAMES_IN_FLIGHT );
  allocInfo.pSetLayouts = layouts.data();

  m_descriptorSets.resize( MAX_FRAMES_IN_FLIGHT );
  if ( vkAllocateDescriptorSets( m_device, &allocInfo, m_descriptorSets.data() ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to allocate descriptor sets!" );
  }

  for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
  {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = m_uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof( UniformBufferoObject );

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = m_textureView;
    imageInfo.sampler = m_textureSampler;

    std::array<VkWriteDescriptorSet, 2> descriptorWrites{
      VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_descriptorSets[i],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &bufferInfo,
      },
      VkWriteDescriptorSet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                            .dstSet = m_descriptorSets[i],
                            .dstBinding = 1,
                            .dstArrayElement = 0,
                            .descriptorCount = 1,
                            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            .pImageInfo = &imageInfo } };

    vkUpdateDescriptorSets(
      m_device, static_cast<uint32_t>( descriptorWrites.size() ), descriptorWrites.data(), 0, nullptr );
  }
}

void VulkanBase::cleanSwapchain()
{
  vkDestroyImage( m_device, m_depthImage, nullptr );
  vkDestroyImageView( m_device, m_depthView, nullptr );
  vkFreeMemory( m_device, m_depthMemory, nullptr );

  vkDestroyImage( m_device, m_textureImage, nullptr );
  vkDestroyImageView( m_device, m_textureView, nullptr );
  vkFreeMemory( m_device, m_textureImageMemory, nullptr );

  // destroy uniform buffers
  for ( auto i = 0u; i < m_uniformBuffers.size(); i++ )
  {
    vkDestroyBuffer( m_device, m_uniformBuffers.at( i ), nullptr );
    vkFreeMemory( m_device, m_uniformBuffersMemory.at( i ), nullptr );
  }

  vkDestroyDescriptorPool( m_device, m_descriptorPool, nullptr );
  vkDestroyDescriptorSetLayout( m_device, m_descriptorSetLayout, nullptr );

  for ( auto imageView : m_swapChainImageViews )
  {
    vkDestroyImageView( m_device, imageView, nullptr );
  }

  vkDestroySwapchainKHR( m_device, m_swapChain, nullptr );
}

void VulkanBase::createSurface()
{
  if ( SDL_Vulkan_CreateSurface( window, m_instance, &m_surface ) != SDL_TRUE )
  {
    throw std::runtime_error( "could not create surface" );
  }
}

void VulkanBase::createSyncObj()
{
  auto size = m_swapChainImages.size();
  m_inFlightFences.resize( size, VK_NULL_HANDLE );
  m_imagesInFlight.resize( size, VK_NULL_HANDLE );
  m_imageAvailableSemaphores.resize( size );
  m_renderingFinishedSemaphores.resize( size );
  for ( auto i = 0u; i < size; i++ )
  {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if ( vkCreateSemaphore( m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores.at( i ) ) != VK_SUCCESS ||
         vkCreateSemaphore( m_device, &semaphoreInfo, nullptr, &m_renderingFinishedSemaphores.at( i ) ) != VK_SUCCESS ||
         vkCreateFence( m_device, &fenceInfo, nullptr, &m_inFlightFences.at( i ) ) != VK_SUCCESS )
    {
      throw std::runtime_error( "failed to create sempahores" );
    }
  }
}

void VulkanBase::createLogicalDevice()
{
  // get the indices of the phisycal device we picked earlier
  QueueFamilyIndices indices = findQueueFamilies( m_physicalDevice );

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

  float queuePriority = 1.0f;
  for ( uint32_t queueFamily : uniqueQueueFamilies )
  {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back( queueCreateInfo );
  }

  // specify the features
  VkPhysicalDeviceFeatures deviceFeatures{};
  VkPhysicalDeviceDynamicRenderingFeatures dynamicRendering{};
  dynamicRendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
  dynamicRendering.dynamicRendering = VK_TRUE;

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pNext = &dynamicRendering;
  createInfo.queueCreateInfoCount = queueCreateInfos.size();
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.pEnabledFeatures = &deviceFeatures;

  createInfo.enabledExtensionCount = 1;
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  // enable validation layers
  if ( enableValidationLayers )
  {
    createInfo.enabledLayerCount = static_cast<uint32_t>( validationLayers.size() );
    createInfo.ppEnabledLayerNames = validationLayers.data();
  }
  else
  {
    createInfo.enabledLayerCount = 0;
  }

  if ( vkCreateDevice( m_physicalDevice, &createInfo, nullptr, &m_device ) != VK_SUCCESS )
  {
    throw std::runtime_error( "could not create logical device" );
  }

  vkGetDeviceQueue( m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue );
  vkGetDeviceQueue( m_device, indices.presentFamily.value(), 0, &m_presentQueue );
}

void VulkanBase::pickPhysicalDevice()
{
  // this should choose the physical device (GPU) we are going to use for the rendering
  uint32_t deviceCount{ 0u };
  vkEnumeratePhysicalDevices( m_instance, &deviceCount, nullptr );

  // check to see if we have at leas one vulkan supporting gpu
  if ( deviceCount == 0 )
  {
    throw std::runtime_error( "failed to find GPU with Vulkan support, i'm sorry" );
  }
  std::vector<VkPhysicalDevice> devices( deviceCount );
  vkEnumeratePhysicalDevices( m_instance, &deviceCount, devices.data() );

  // we will implement a multimap with device scores later
  // the score is based on properties and features of the gpu, the more features and
  // the better properties it has, the higher the score
  for ( auto& device : devices )
  {
    if ( isDeviceSuitable( device ) )
    {
      VkPhysicalDeviceProperties props;
      vkGetPhysicalDeviceProperties( device, &props );
      spdlog::info( "found {}", props.deviceName );
      m_physicalDevice = device;
      break;
    }
  }
}

bool VulkanBase::isDeviceSuitable( VkPhysicalDevice& device )
{
  // get the device features and properties
  vkGetPhysicalDeviceProperties( device, &m_physicalDeviceProps );
  vkGetPhysicalDeviceFeatures( device, &m_physicalDeviceFeatures );
  vkGetPhysicalDeviceMemoryProperties( device, &m_physicalDeviceMemoryProps );
  auto indices = findQueueFamilies( device );
  auto extensionsSupported = checkDeviceExtensionSupport( device );

  // check to see swap chain support
  bool swapChainAdequate = false;
  if ( extensionsSupported )
  {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport( device );
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  if ( m_physicalDeviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
       m_physicalDeviceFeatures.geometryShader && indices.isComplete() && extensionsSupported && swapChainAdequate )
    return true;

  return false;
}

bool VulkanBase::checkDeviceExtensionSupport( VkPhysicalDevice& device )
{
  // get all the extensions of the device
  uint32_t extensionCount{ 0u };
  vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, nullptr );
  std::vector<VkExtensionProperties> availableExtensions( extensionCount );
  vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, availableExtensions.data() );

  // fill the set with the global deviceExtensions
  std::set<std::string> requiredExtensions( deviceExtensions.begin(), deviceExtensions.end() );

  // if the extension of the device is present in the set, or the extensions of the device contain the global
  // extension too, it will get erased and check if the required is empty
  for ( const auto& extension : availableExtensions )
  {
    requiredExtensions.erase( extension.extensionName );
  }

  return requiredExtensions.empty();
}

auto VulkanBase::querySwapChainSupport( VkPhysicalDevice& device ) -> SwapChainSupportDetails
{
  SwapChainSupportDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, m_surface, &details.capabilities );
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR( device, m_surface, &formatCount, nullptr );

  if ( formatCount != 0 )
  {
    details.formats.resize( formatCount );
    vkGetPhysicalDeviceSurfaceFormatsKHR( device, m_surface, &formatCount, details.formats.data() );
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR( device, m_surface, &presentModeCount, nullptr );

  if ( presentModeCount != 0 )
  {
    details.presentModes.resize( presentModeCount );
    vkGetPhysicalDeviceSurfacePresentModesKHR( device, m_surface, &presentModeCount, details.presentModes.data() );
  }

  return details;
}

auto VulkanBase::chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities ) -> VkExtent2D
{
  if ( capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() )
  {
    return capabilities.currentExtent;
  }
  else
  {
    int width, height;
    SDL_GetWindowSize( window, &width, &height );

    VkExtent2D actualExtent = { static_cast<uint32_t>( width ), static_cast<uint32_t>( height ) };

    actualExtent.width =
      std::clamp( actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width );
    actualExtent.height =
      std::clamp( actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height );

    return actualExtent;
  }
}

auto VulkanBase::chooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes ) -> VkPresentModeKHR
{
  for ( const auto& availablePresentMode : availablePresentModes )
  {
    if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
    {
      return availablePresentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

void VulkanBase::createSwapChain()
{
  SwapChainSupportDetails swapChainSupport = querySwapChainSupport( m_physicalDevice );
  auto surfaceFormat = chooseSwapSurfaceFormat( swapChainSupport.formats );
  auto presentMode = chooseSwapPresentMode( swapChainSupport.presentModes );
  auto extent = chooseSwapExtent( swapChainSupport.capabilities );
  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

  if ( swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount )
  {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = m_surface;

  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  // we need VK_IMAGE_USAGE_TRANSFER_DST_BIT if we want to do post processing as this is a color attachment
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  auto indices = findQueueFamilies( m_physicalDevice );
  uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

  if ( indices.graphicsFamily != indices.presentFamily )
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  }
  else
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
  }

  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if ( vkCreateSwapchainKHR( m_device, &createInfo, nullptr, &m_swapChain ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create swap chain!" );
  }

  vkGetSwapchainImagesKHR( m_device, m_swapChain, &imageCount, nullptr );
  m_swapChainImages.resize( imageCount );
  vkGetSwapchainImagesKHR( m_device, m_swapChain, &imageCount, m_swapChainImages.data() );

  m_swapChainImageFormat = surfaceFormat.format;
  m_swapChainExtent = extent;
}

void VulkanBase::createImageViews()
{
  m_swapChainImageViews.resize( m_swapChainImages.size() );
  for ( auto i = 0u; i < m_swapChainImageViews.size(); i++ )
  {
    m_swapChainImageViews.at( i ) =
      createImageView( m_swapChainImages.at( i ), m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT );
  }
}

void VulkanBase::createDepthResources()
{
  auto depthFormat = findDepthFormat();
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = m_swapChainExtent.width;
  imageInfo.extent.height = m_swapChainExtent.height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = depthFormat;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  createImage( m_swapChainExtent.width,
               m_swapChainExtent.height,
               depthFormat,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
               m_depthImage,
               m_depthMemory );

  m_depthView = createImageView( m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT );
}

void VulkanBase::createTextureImage()
{
#define pic "D:/Github/cpp_playground/test.jpg"
  int texWidth, texHeight, texChannels;
  stbi_uc* pixels = stbi_load( pic, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha );

  if ( !pixels )
  {
    spdlog::error( stbi_failure_reason() );
    throw std::runtime_error( "failed to load texture image!" );
  }

  VkDeviceSize imageSize = texWidth * texHeight * 4;

  createBuffer( imageSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_stageBuffer,
                m_stageBufferMemory );

  void* data;
  vkMapMemory( m_device, m_stageBufferMemory, 0, imageSize, 0, &data );
  memcpy( data, pixels, static_cast<size_t>( imageSize ) );
  vkUnmapMemory( m_device, m_stageBufferMemory );

  stbi_image_free( pixels );

  createImage( texWidth,
               texHeight,
               VK_FORMAT_R8G8B8A8_SRGB,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
               m_textureImage,
               m_textureImageMemory );

  transitionImageLayout(
    m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
  copyBufferToImage(
    m_stageBuffer, m_textureImage, static_cast<uint32_t>( texWidth ), static_cast<uint32_t>( texHeight ) );
  transitionImageLayout( m_textureImage,
                         VK_FORMAT_R8G8B8A8_SRGB,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

  vkDestroyBuffer( m_device, m_stageBuffer, nullptr );
  vkFreeMemory( m_device, m_stageBufferMemory, nullptr );
}

void VulkanBase::createTextureSamplers()
{
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties( m_physicalDevice, &properties );

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

  if ( vkCreateSampler( m_device, &samplerInfo, nullptr, &m_textureSampler ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create texture sampler!" );
  }
}

void VulkanBase::createBuffer( VkDeviceSize size,
                               VkBufferUsageFlags usage,
                               VkMemoryPropertyFlags properties,
                               VkBuffer& buffer,
                               VkDeviceMemory& bufferMemory )
{
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if ( vkCreateBuffer( m_device, &bufferInfo, nullptr, &buffer ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create buffer!" );
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements( m_device, buffer, &memRequirements );

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType( memRequirements.memoryTypeBits, properties );

  if ( vkAllocateMemory( m_device, &allocInfo, nullptr, &bufferMemory ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to allocate buffer memory!" );
  }

  vkBindBufferMemory( m_device, buffer, bufferMemory, 0 );
}

auto VulkanBase::chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats )
  -> VkSurfaceFormatKHR
{
  for ( const auto& availableFormat : availableFormats )
  {
    if ( availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
         availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
    {
      return availableFormat;
    }
  }
  return availableFormats.at( 0 );
}

void VulkanBase::run()
{
  initVulkan();
  mainLoop();
}

void VulkanBase::createGraphicsPipeline()
{
  auto vertShaderCode = readFile( "D:/Github/cpp_playground/vert.spv" );
  auto fragShaderCode = readFile( "D:/Github/cpp_playground/frag.spv" );

  auto vertModule = createShaderModule( vertShaderCode );
  auto fragModule = createShaderModule( fragShaderCode );

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

  auto bindingDesc = Vertex::getBindingDescription();
  auto attribDesc = Vertex::getAttributeDescriptions();

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &bindingDesc,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>( attribDesc.size() ),
    .pVertexAttributeDescriptions = attribDesc.data(),
  };

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>( dynamicStates.size() );
  dynamicState.pDynamicStates = dynamicStates.data();

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                                                 .setLayoutCount = 1,
                                                 .pSetLayouts = &m_descriptorSetLayout };

  if ( vkCreatePipelineLayout( m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create pipeline layout!" );
  }

  VkPipelineRenderingCreateInfo renderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachmentFormats = &m_swapChainImageFormat;
  renderingInfo.depthAttachmentFormat = findDepthFormat();
  renderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

  VkPipelineDepthStencilStateCreateInfo depthStencil{ .sType =
                                                        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                                                      .depthTestEnable = VK_TRUE,
                                                      .depthWriteEnable = VK_TRUE,
                                                      .depthCompareOp = VK_COMPARE_OP_LESS,
                                                      .depthBoundsTestEnable = VK_FALSE,
                                                      .stencilTestEnable = VK_FALSE };

  VkGraphicsPipelineCreateInfo pipelineInfo{
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = &renderingInfo,
    .stageCount = 2,
    .pStages = shaderStages,
    .pVertexInputState = &vertexInputInfo,
    .pInputAssemblyState = &inputAssembly,
    .pViewportState = &viewportState,
    .pRasterizationState = &rasterizer,
    .pMultisampleState = &multisampling,
    .pDepthStencilState = &depthStencil,
    .pColorBlendState = &colorBlending,
    .pDynamicState = &dynamicState,
    .layout = m_pipelineLayout,
    .renderPass = VK_NULL_HANDLE,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
  };

  if ( vkCreateGraphicsPipelines( m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline ) !=
       VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create graphics pipeline!" );
  }
  vkDestroyShaderModule( m_device, vertModule, nullptr );
  vkDestroyShaderModule( m_device, fragModule, nullptr );
}

auto VulkanBase::createShaderModule( const std::vector<char>& code ) const -> VkShaderModule
{
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>( code.data() );
  VkShaderModule shaderModule;
  if ( vkCreateShaderModule( m_device, &createInfo, nullptr, &shaderModule ) )
  {
    throw std::runtime_error( "failed to create shader module" );
  }
  return shaderModule;
}

void VulkanBase::createImguiPipeline()
{
  auto vertShaderCode = readFile( "F:/github/cpp_playground/vulkan/glsl_shader.vert.u32" );
  auto fragShaderCode = readFile( "F:/github/cpp_playground/vulkan/glsl_shader.frag.u32" );

  auto vertModule = createShaderModule( vertShaderCode );
  auto fragModule = createShaderModule( fragShaderCode );

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

  auto bindingDesc = Vertex::getBindingDescription();
  auto attribDesc = Vertex::getAttributeDescriptions();

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &bindingDesc,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>( attribDesc.size() ),
    .pVertexAttributeDescriptions = attribDesc.data(),
  };

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>( dynamicStates.size() );
  dynamicState.pDynamicStates = dynamicStates.data();

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                                                 .setLayoutCount = 1,
                                                 .pSetLayouts = &m_descriptorSetLayout };

  if ( vkCreatePipelineLayout( m_device, &pipelineLayoutInfo, nullptr, &m_imguiPipelineLayout ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create pipeline layout!" );
  }

  VkPipelineRenderingCreateInfo renderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachmentFormats = &m_swapChainImageFormat;
  renderingInfo.depthAttachmentFormat = findDepthFormat();
  renderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

  VkPipelineDepthStencilStateCreateInfo depthStencil{ .sType =
                                                        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                                                      .depthTestEnable = VK_TRUE,
                                                      .depthWriteEnable = VK_TRUE,
                                                      .depthCompareOp = VK_COMPARE_OP_LESS,
                                                      .depthBoundsTestEnable = VK_FALSE,
                                                      .stencilTestEnable = VK_FALSE };

  VkGraphicsPipelineCreateInfo pipelineInfo{
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = &renderingInfo,
    .stageCount = 2,
    .pStages = shaderStages,
    .pVertexInputState = &vertexInputInfo,
    .pInputAssemblyState = &inputAssembly,
    .pViewportState = &viewportState,
    .pRasterizationState = &rasterizer,
    .pMultisampleState = &multisampling,
    .pDepthStencilState = &depthStencil,
    .pColorBlendState = &colorBlending,
    .pDynamicState = &dynamicState,
    .layout = m_pipelineLayout,
    .renderPass = VK_NULL_HANDLE,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
  };

  if ( vkCreateGraphicsPipelines( m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_imguiPipeline ) !=
       VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create graphics pipeline!" );
  }
  vkDestroyShaderModule( m_device, vertModule, nullptr );
  vkDestroyShaderModule( m_device, fragModule, nullptr );
}

void VulkanBase::createInstance()
{
  if ( enableValidationLayers && !checkValidationLayerSupport() )
  {
    throw std::runtime_error( "validation layers requested but not available!!!" );
  }
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Hello Triangle";
  appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
  appInfo.apiVersion = VK_API_VERSION_1_4;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  auto extensions = getRequiredExtensions();
  createInfo.enabledExtensionCount = static_cast<uint32_t>( extensions.size() );
  createInfo.ppEnabledExtensionNames = extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if ( enableValidationLayers )
  {
    createInfo.enabledLayerCount = static_cast<uint32_t>( validationLayers.size() );
    createInfo.ppEnabledLayerNames = validationLayers.data();

    populateDebugMessengerCreateInfo( debugCreateInfo );
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
  }
  else
  {
    createInfo.enabledLayerCount = 0;

    createInfo.pNext = nullptr;
  }

  for ( auto& extension : extensions )
  {
    spdlog::info( "extension: {}", extension );
  }

  if ( vkCreateInstance( &createInfo, nullptr, &m_instance ) != VK_SUCCESS )
  {
    throw std::runtime_error( "cannot create instance" );
  }
}

void VulkanBase::initWindow()
{
  if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) != 0 )
  {
    spdlog::error( "SDL_Init Error: {}", SDL_GetError() );
    throw std::runtime_error( "SDL_Init failed" );
  }

  if ( SDL_Vulkan_LoadLibrary( nullptr ) != 0 )
  {
    spdlog::error( "SDL Vulkan load failed: {}", SDL_GetError() );
    throw std::runtime_error( "could not load lib vulkan" );
  }

  window =
    SDL_CreateWindow( "kogayonon", 100, 100, 800, 800, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN );
}

void VulkanBase::drawFrame()
{
  vkWaitForFences( m_device, 1, &m_inFlightFences.at( m_currentFrame ), VK_TRUE, UINT64_MAX );

  uint32_t imageIndex;
  auto result = vkAcquireNextImageKHR(
    m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphores.at( m_currentFrame ), VK_NULL_HANDLE, &imageIndex );

  if ( result == VK_ERROR_OUT_OF_DATE_KHR )
  {
    recreateSwapchain();
    return;
  }
  else if ( result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR )
  {
    throw std::runtime_error( "failed to acquire swap chain image!" );
  }

  vkResetFences( m_device, 1, &m_inFlightFences.at( m_currentFrame ) );
  updateUniformBuffer( imageIndex );
  // vkResetCommandBuffer( m_commandBuffers.at( m_currentFrame ), 0 );
  recordCommandBuffer( m_commandBuffers.at( m_currentFrame ), imageIndex );

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &m_imageAvailableSemaphores.at( m_currentFrame );
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &m_commandBuffers.at( m_currentFrame );

  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &m_renderingFinishedSemaphores.at( imageIndex );

  if ( vkQueueSubmit( m_graphicsQueue, 1, &submitInfo, m_inFlightFences.at( m_currentFrame ) ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to submit draw command buffer!" );
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &m_renderingFinishedSemaphores.at( imageIndex );

  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &m_swapChain;

  presentInfo.pImageIndices = &imageIndex;

  vkQueuePresentKHR( m_presentQueue, &presentInfo );

  m_currentFrame = ( m_currentFrame + 1 ) % 2;
}

void VulkanBase::mainLoop()
{
  while ( m_running )
  {
    drawFrame();
    SDL_Event e;
    while ( SDL_PollEvent( &e ) )
    {
      ImGui_ImplSDL2_ProcessEvent( &e );
      switch ( e.type )
      {
      case SDL_KEYDOWN:
        if ( e.key.keysym.scancode == SDL_SCANCODE_DELETE )
        {
          m_running = false;
        }
        break;
      case SDL_WINDOWEVENT:
        if ( e.window.event == SDL_WINDOWEVENT_RESIZED )
        {
          recreateSwapchain();
        }
        break;
      case SDL_QUIT:
        m_running = false;
      }
    }
  }

  vkDeviceWaitIdle( m_device );
}

void VulkanBase::setupDebug()
{
  if ( !enableValidationLayers )
    return;

  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  populateDebugMessengerCreateInfo( createInfo );
  if ( CreateDebugUtilsMessengerEXT( m_instance, &createInfo, nullptr, &debugMessenger ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to set up debug messenger!" );
  }
}

bool VulkanBase::checkValidationLayerSupport()
{
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

  std::vector<VkLayerProperties> availableLayers( layerCount );
  vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

  for ( const char* layerName : validationLayers )
  {
    bool layerFound = false;

    for ( const auto& layerProperties : availableLayers )
    {
      if ( strcmp( layerName, layerProperties.layerName ) == 0 )
      {
        layerFound = true;
        break;
      }
    }

    if ( !layerFound )
    {
      return false;
    }
  }

  return true;
}

void VulkanBase::populateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT& info )
{
  info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  info.pfnUserCallback = debugCallback;
  info.pNext = nullptr;
}

auto VulkanBase::findQueueFamilies( VkPhysicalDevice& device ) -> QueueFamilyIndices
{
  QueueFamilyIndices indices;
  uint32_t queueFamilyCount{ 0u };
  vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, nullptr );
  std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
  vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies.data() );
  int i = 0;
  for ( const auto& queueFamily : queueFamilies )
  {
    // we need to find at least one queue family that supports the graphics bit
    if ( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
    {
      indices.graphicsFamily = i;
    }

    VkBool32 presentSupport{ false };
    vkGetPhysicalDeviceSurfaceSupportKHR( device, i, m_surface, &presentSupport );

    if ( presentSupport )
    {
      indices.presentFamily = i;
    }

    // early exit if we already found a family
    if ( indices.isComplete() )
      break;

    i++;
  }
  return indices;
}

void VulkanBase::cleanup()
{
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  cleanSwapchain();
  vkDestroyBuffer( m_device, m_vertexBuffer, nullptr );
  vkFreeMemory( m_device, m_vertexBufferMemory, nullptr );
  vkFreeMemory( m_device, m_indicesBufferMemory, nullptr );
  vkDestroyBuffer( m_device, m_indicesBuffer, nullptr );

  vkDestroySampler( m_device, m_textureSampler, nullptr );
  vkDestroyImage( m_device, m_textureImage, nullptr );
  vkDestroyImageView( m_device, m_textureView, nullptr );
  vkFreeMemory( m_device, m_textureImageMemory, nullptr );

  for ( size_t i = 0; i < 3; i++ )
  {
    vkDestroyBuffer( m_device, m_uniformBuffers[i], nullptr );
    vkFreeMemory( m_device, m_uniformBuffersMemory[i], nullptr );
  }

  vkDestroyDescriptorPool( m_device, m_descriptorPool, nullptr );

  vkDestroyDescriptorSetLayout( m_device, m_descriptorSetLayout, nullptr );

  vkDestroyDevice( m_device, nullptr );

  if ( enableValidationLayers )
  {
    DestroyDebugUtilsMessengerEXT( m_instance, debugMessenger, nullptr );
  }

  vkDestroySurfaceKHR( m_instance, m_surface, nullptr );
  vkDestroyInstance( m_instance, nullptr );
  SDL_DestroyWindow( window );

  SDL_Quit();
}

auto VulkanBase::getRequiredExtensions() -> std::vector<const char*>
{
  uint32_t extensionCount{ 0u };
  SDL_Vulkan_GetInstanceExtensions( window, &extensionCount, nullptr );
  spdlog::info( extensionCount );
  std::vector<const char*> extensions( extensionCount );
  SDL_Vulkan_GetInstanceExtensions( window, &extensionCount, extensions.data() );

  if ( enableValidationLayers )
    extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );

  return extensions;
}

void VulkanBase::createImage( uint32_t width,
                              uint32_t height,
                              VkFormat format,
                              VkImageTiling tiling,
                              VkImageUsageFlags usage,
                              VkMemoryPropertyFlags properties,
                              VkImage& image,
                              VkDeviceMemory& imageMemory )
{
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if ( vkCreateImage( m_device, &imageInfo, nullptr, &image ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create image!" );
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements( m_device, image, &memRequirements );

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType( memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

  if ( vkAllocateMemory( m_device, &allocInfo, nullptr, &imageMemory ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to allocate image memory!" );
  }

  vkBindImageMemory( m_device, image, imageMemory, 0 );
}

auto VulkanBase::findMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties ) -> uint32_t
{
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties( m_physicalDevice, &memProperties );

  for ( uint32_t i = 0; i < memProperties.memoryTypeCount; i++ )
  {
    if ( ( typeFilter & ( 1 << i ) ) && ( memProperties.memoryTypes[i].propertyFlags & properties ) == properties )
    {
      return i;
    }
  }

  throw std::runtime_error( "failed to find suitable memory type!" );
}

auto VulkanBase::findSupportedFormat( const std::vector<VkFormat>& candidates,
                                      VkImageTiling tiling,
                                      VkFormatFeatureFlags features ) -> VkFormat
{
  for ( VkFormat format : candidates )
  {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties( m_physicalDevice, format, &props );

    if ( tiling == VK_IMAGE_TILING_LINEAR && ( props.linearTilingFeatures & features ) == features )
    {
      return format;
    }
    else if ( tiling == VK_IMAGE_TILING_OPTIMAL && ( props.optimalTilingFeatures & features ) == features )
    {
      return format;
    }
  }

  throw std::runtime_error( "failed to find supported format!" );
}

auto VulkanBase::findDepthFormat() -> VkFormat
{
  return findSupportedFormat( { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
                              VK_IMAGE_TILING_OPTIMAL,
                              VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
}

auto VulkanBase::createImageView( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags ) -> VkImageView
{
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  VkImageView imageView;
  if ( vkCreateImageView( m_device, &viewInfo, nullptr, &imageView ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create image view!" );
  }

  return imageView;
}
