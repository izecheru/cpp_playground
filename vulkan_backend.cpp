#include "vulkan_backend.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <spdlog/spdlog.h>

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
  createGraphicsPipeline();
  createCommandPool();
  createCommandBuffer();
  createSyncObj();
}

void VulkanBase::recordCommandBuffer( VkCommandBuffer& cmd, uint32_t imageIndex )
{
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  vkBeginCommandBuffer( cmd, &beginInfo );

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barrier.image = m_swapChainImages[imageIndex];
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.layerCount = 1;

  vkCmdPipelineBarrier( cmd,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &barrier );

  VkRenderingAttachmentInfo colorAttachment{};
  colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  colorAttachment.imageView = m_swapChainImageViews[imageIndex];
  colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.clearValue = { { 0.f, 0.f, 0.f, 1.f } };

  VkRenderingInfo renderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderingInfo.renderArea = { { 0, 0 }, m_swapChainExtent };
  renderingInfo.layerCount = 1;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachments = &colorAttachment;

  vkCmdBeginRendering( cmd, &renderingInfo );

  vkCmdBindPipeline( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline );

  VkViewport viewport{ 0, 0, (float)m_swapChainExtent.width, (float)m_swapChainExtent.height, 0.0f, 1.0f };
  vkCmdSetViewport( cmd, 0, 1, &viewport );

  VkRect2D scissor{ { 0, 0 }, m_swapChainExtent };
  vkCmdSetScissor( cmd, 0, 1, &scissor );

  vkCmdDraw( cmd, 3, 1, 0, 0 );

  vkCmdEndRendering( cmd );

  barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barrier.dstAccessMask = 0;

  vkCmdPipelineBarrier( cmd,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &barrier );

  vkEndCommandBuffer( cmd );
}

void VulkanBase::createCommandBuffer()
{
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = m_commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  if ( vkAllocateCommandBuffers( m_device, &allocInfo, &m_commandBuffer ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to allocate command buffers!" );
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
    throw std::runtime_error( "failed to open file!\n" );
  }
  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer( fileSize );
  file.seekg( 0 );
  file.read( buffer.data(), fileSize );
  file.close();

  return buffer;
}

void VulkanBase::cleanSwapchain()
{
  vkDestroySemaphore( m_device, m_imageAvailableSemaphore, nullptr );
  vkDestroySemaphore( m_device, m_renderFinishedSemaphore, nullptr );
  vkDestroyFence( m_device, m_inFlightFence, nullptr );

  vkDestroyCommandPool( m_device, m_commandPool, nullptr );

  vkDestroyPipeline( m_device, m_graphicsPipeline, nullptr );
  vkDestroyPipelineLayout( m_device, m_pipelineLayout, nullptr );

  for ( auto imageView : m_swapChainImageViews )
  {
    vkDestroyImageView( m_device, imageView, nullptr );
  }

  vkDestroySwapchainKHR( m_device, m_swapChain, nullptr );
}

void VulkanBase::createSurface()
{
  if ( glfwCreateWindowSurface( m_instance, window, nullptr, &m_surface ) != VK_SUCCESS )
  {
    throw std::runtime_error( "could not create surface" );
  }
}

void VulkanBase::createSyncObj()
{
  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  if ( vkCreateSemaphore( m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore ) != VK_SUCCESS ||
       vkCreateSemaphore( m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore ) != VK_SUCCESS ||
       vkCreateFence( m_device, &fenceInfo, nullptr, &m_inFlightFence ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create sempahores" );
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
  VkPhysicalDeviceProperties deviceProperties;
  VkPhysicalDeviceFeatures deviceFeatures;
  // get the device features and properties
  vkGetPhysicalDeviceProperties( device, &deviceProperties );
  vkGetPhysicalDeviceFeatures( device, &deviceFeatures );
  auto indices = findQueueFamilies( device );
  auto extensionsSupported = checkDeviceExtensionSupport( device );

  // check to see swap chain support
  bool swapChainAdequate = false;
  if ( extensionsSupported )
  {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport( device );
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  spdlog::info( "extensions supported? {}\nswapchain adequate?{}", extensionsSupported, swapChainAdequate );

  if ( deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader &&
       indices.isComplete() && extensionsSupported && swapChainAdequate )
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
    glfwGetFramebufferSize( window, &width, &height );

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
  for ( size_t i = 0; i < m_swapChainImages.size(); i++ )
  {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = m_swapChainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = m_swapChainImageFormat;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;
    if ( vkCreateImageView( m_device, &createInfo, nullptr, &m_swapChainImageViews[i] ) != VK_SUCCESS )
    {
      throw std::runtime_error( "failed to create image views!" );
    }
  }
}

auto VulkanBase::chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats )
  -> VkSurfaceFormatKHR
{
  for ( const auto& availableFormat : availableFormats )
  {
    if ( availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
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
  auto vertShaderCode = readFile( "F:/github/test/vert.spv" );
  auto fragShaderCode = readFile( "F:/github/test/frag.spv" );

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

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;

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

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

  if ( vkCreatePipelineLayout( m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create pipeline layout!" );
  }

  VkPipelineRenderingCreateInfo renderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachmentFormats = &m_swapChainImageFormat;

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = m_pipelineLayout;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.renderPass = VK_NULL_HANDLE;
  pipelineInfo.pNext = &renderingInfo;

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
  glfwInit();

  glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
  glfwWindowHint( GLFW_RESIZABLE, GLFW_TRUE );

  window = glfwCreateWindow( 800, 800, "Vulkan", nullptr, nullptr );

  glfwSetWindowUserPointer( window, this );

  glfwSetWindowSizeCallback( window, []( GLFWwindow* window, int w, int h ) {
    VulkanBase* app = reinterpret_cast<VulkanBase*>( glfwGetWindowUserPointer( window ) );
    app->cleanSwapchain();
    app->createSwapChain();
    app->createImageViews();
    app->createGraphicsPipeline();
    app->createCommandPool();
    app->createCommandBuffer();
    app->createSyncObj();
  } );
}

void VulkanBase::drawFrame()
{
  vkWaitForFences( m_device, 1, &m_inFlightFence, VK_TRUE, UINT64_MAX );
  vkResetFences( m_device, 1, &m_inFlightFence );

  uint32_t imageIndex;
  // use swapchainImages[imageIndex] image this frame
  vkAcquireNextImageKHR( m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex );

  vkResetCommandBuffer( m_commandBuffer, 0 );

  recordCommandBuffer( m_commandBuffer, imageIndex );

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore };
  VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &m_commandBuffer;

  VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphore };
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if ( vkQueueSubmit( m_graphicsQueue, 1, &submitInfo, m_inFlightFence ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to submit draw command buffer!" );
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = { m_swapChain };
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;

  presentInfo.pImageIndices = &imageIndex;

  vkQueuePresentKHR( m_presentQueue, &presentInfo );
}

void VulkanBase::mainLoop()
{
  while ( !glfwWindowShouldClose( window ) )
  {
    glfwPollEvents();
    drawFrame();
    if ( glfwGetKey( window, GLFW_KEY_ESCAPE ) == GLFW_PRESS )
    {
      spdlog::info( "closing app" );
      glfwSetWindowShouldClose( window, true );
    }
    vkDeviceWaitIdle( m_device );
  }
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
  vkDestroySemaphore( m_device, m_imageAvailableSemaphore, nullptr );
  vkDestroySemaphore( m_device, m_renderFinishedSemaphore, nullptr );
  vkDestroyFence( m_device, m_inFlightFence, nullptr );

  vkDestroyCommandPool( m_device, m_commandPool, nullptr );
  vkDestroyPipeline( m_device, m_graphicsPipeline, nullptr );
  vkDestroyPipelineLayout( m_device, m_pipelineLayout, nullptr );

  for ( auto imageView : m_swapChainImageViews )
  {
    vkDestroyImageView( m_device, imageView, nullptr );
  }

  vkDestroySwapchainKHR( m_device, m_swapChain, nullptr );
  vkDestroyDevice( m_device, nullptr );

  if ( enableValidationLayers )
  {
    DestroyDebugUtilsMessengerEXT( m_instance, debugMessenger, nullptr );
  }

  vkDestroySurfaceKHR( m_instance, m_surface, nullptr );
  vkDestroyInstance( m_instance, nullptr );
  glfwDestroyWindow( window );

  glfwTerminate();
}

auto VulkanBase::getRequiredExtensions() -> std::vector<const char*>
{
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

  std::vector<const char*> extensions( glfwExtensions, glfwExtensions + glfwExtensionCount );

  if ( enableValidationLayers )
  {
    extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
  }

  return extensions;
}