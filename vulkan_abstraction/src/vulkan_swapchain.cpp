#include "vulkan_swapchain/vulkan_swapchain.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include "vulkan_device/vulkan_device.hpp"

auto VulkanSwapchain::querySwapchainSupport() -> SwapchainSupportDetails
{
  SwapchainSupportDetails details;
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(
    m_pDevice->getPhysicalDevice(), m_pDevice->getSurface(), &formatCount, nullptr );

  if ( formatCount != 0 )
  {
    details.formats.resize( formatCount );
    vkGetPhysicalDeviceSurfaceFormatsKHR(
      m_pDevice->getPhysicalDevice(), m_pDevice->getSurface(), &formatCount, details.formats.data() );
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(
    m_pDevice->getPhysicalDevice(), m_pDevice->getSurface(), &presentModeCount, nullptr );

  if ( presentModeCount != 0 )
  {
    details.presentModes.resize( presentModeCount );
    vkGetPhysicalDeviceSurfacePresentModesKHR(
      m_pDevice->getPhysicalDevice(), m_pDevice->getSurface(), &presentModeCount, details.presentModes.data() );
  }

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    m_pDevice->getPhysicalDevice(), m_pDevice->getSurface(), &details.capabilities );

  return details;
}

VulkanSwapchain::VulkanSwapchain( std::shared_ptr<VulkanDevice> device )
    : m_pDevice{ device.get() }
{
  createSwapchain();
  createImageViews();
  // createRenderPass();
  // createFramebuffers();
  createSyncObjects();
}

VulkanSwapchain::~VulkanSwapchain()
{
  destroy();
}

void VulkanSwapchain::destroy()
{
  vkDeviceWaitIdle( m_pDevice->getLogicalDevice() );

  vkDestroySemaphore( m_pDevice->getLogicalDevice(), m_imageAvailableSemaphore, nullptr );
  vkDestroySemaphore( m_pDevice->getLogicalDevice(), m_renderFinishedSemaphore, nullptr );
  vkDestroyFence( m_pDevice->getLogicalDevice(), m_inFlightFence, nullptr );

  vkDestroyRenderPass( m_pDevice->getLogicalDevice(), m_renderPass, nullptr );
  for ( auto& fb : m_framebuffers )
  {
    vkDestroyFramebuffer( m_pDevice->getLogicalDevice(), fb, nullptr );
  }
  for ( auto& swapchainImage : m_swapchainImages )
  {
    vkDestroyImageView( m_pDevice->getLogicalDevice(), swapchainImage.imageView, nullptr );
  }

  vkDestroySwapchainKHR( m_pDevice->getLogicalDevice(), m_swapchain, nullptr );
}

void VulkanSwapchain::createSwapchain()
{
  auto details = querySwapchainSupport();
  auto surfaceFormat = chooseSwapSurfaceFormat( details.formats );
  auto presentMode = chooseSwapPresentMode( details.presentModes );
  auto extent = chooseSwapExtent( details.capabilities );

  m_width = extent.width;
  m_height = extent.height;

  uint32_t imageCount = details.capabilities.minImageCount + 1;
  if ( details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount )
  {
    imageCount = details.capabilities.maxImageCount;
  }

  spdlog::info( "max image count {}", imageCount );

  auto& surface = m_pDevice->getSurface();
  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;

  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  auto presentQueue = m_pDevice->getPresentQueue();
  auto graphicsQueue = m_pDevice->getGraphicsQueue();
  uint32_t queueFamilyIndices[] = { graphicsQueue.familyIndex, presentQueue.familyIndex };

  if ( graphicsQueue.familyIndex != presentQueue.familyIndex )
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  }
  else
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  createInfo.preTransform = details.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if ( vkCreateSwapchainKHR( m_pDevice->getLogicalDevice(), &createInfo, nullptr, &m_swapchain ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create swap chain!" );
  }

  vkGetSwapchainImagesKHR( m_pDevice->getLogicalDevice(), m_swapchain, &imageCount, nullptr );
  m_swapchainImages.resize( imageCount );
  std::vector<VkImage> images( imageCount );
  vkGetSwapchainImagesKHR( m_pDevice->getLogicalDevice(), m_swapchain, &imageCount, images.data() );
  for ( auto i = 0u; i < imageCount; i++ )
  {
    m_swapchainImages.at( i ).image = images.at( i );
  }

  m_swapchainFormat = surfaceFormat.format;
  m_swapchainExtent = extent;
}

void VulkanSwapchain::createImageViews()
{
  for ( size_t i = 0; i < m_swapchainImages.size(); i++ )
  {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = m_swapchainImages[i].image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = m_swapchainFormat;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    if ( vkCreateImageView( m_pDevice->getLogicalDevice(), &createInfo, nullptr, &m_swapchainImages[i].imageView ) !=
         VK_SUCCESS )
    {
      throw std::runtime_error( "failed to create image views!" );
    }
  }
}

void VulkanSwapchain::createSyncObjects()
{
  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  if ( vkCreateSemaphore( m_pDevice->getLogicalDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphore ) !=
         VK_SUCCESS ||
       vkCreateSemaphore( m_pDevice->getLogicalDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphore ) !=
         VK_SUCCESS ||
       vkCreateFence( m_pDevice->getLogicalDevice(), &fenceInfo, nullptr, &m_inFlightFence ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create sempahores" );
  }
  spdlog::info( "created sync objects" );
}

void VulkanSwapchain::createFramebuffers()
{
  m_framebuffers.resize( m_swapchainImages.size() );
  for ( size_t i = 0; i < m_swapchainImages.size(); i++ )
  {
    VkImageView attachments[] = { m_swapchainImages[i].imageView };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = m_renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = m_swapchainExtent.width;
    framebufferInfo.height = m_swapchainExtent.height;
    framebufferInfo.layers = 1;

    if ( vkCreateFramebuffer( m_pDevice->getLogicalDevice(), &framebufferInfo, nullptr, &m_framebuffers[i] ) !=
         VK_SUCCESS )
    {
      throw std::runtime_error( "failed to create framebuffer!" );
    }
  }
}

// void VulkanSwapchain::createRenderPass()
//{
//   VkAttachmentDescription colorAttachment{};
//   colorAttachment.format = m_swapchainFormat;
//   colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
//   colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//   colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//   colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//   colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//   colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//   colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
//
//   VkAttachmentReference colorAttachmentRef{};
//   colorAttachmentRef.attachment = 0;
//   colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//   VkSubpassDescription subpass{};
//   subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//   subpass.colorAttachmentCount = 1;
//   subpass.pColorAttachments = &colorAttachmentRef;
//
//   VkRenderPassCreateInfo renderPassInfo{};
//   renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//   renderPassInfo.attachmentCount = 1;
//   renderPassInfo.pAttachments = &colorAttachment;
//   renderPassInfo.subpassCount = 1;
//   renderPassInfo.pSubpasses = &subpass;
//
//   if ( vkCreateRenderPass( m_pDevice->getLogicalDevice(), &renderPassInfo, nullptr, &m_renderPass ) != VK_SUCCESS )
//   {
//     throw std::runtime_error( "failed to create render pass!" );
//   }
// }

auto VulkanSwapchain::getImageView( uint32_t index ) -> VkImageView
{
  return m_swapchainImages.at( index ).imageView;
}

auto VulkanSwapchain::getNextImageIndex() -> uint32_t
{
  uint32_t imageIndex{ 0u };
  vkAcquireNextImageKHR(
    m_pDevice->getLogicalDevice(), m_swapchain, UINT64_MAX, m_imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex );
  return imageIndex;
}

auto VulkanSwapchain::chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities ) -> VkExtent2D
{
  if ( capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() )
  {
    return capabilities.currentExtent;
  }
  else
  {
    int width, height;
    glfwGetFramebufferSize( m_pDevice->getGlfwWindow(), &width, &height );

    VkExtent2D actualExtent = { static_cast<uint32_t>( width ), static_cast<uint32_t>( height ) };

    actualExtent.width =
      std::clamp( actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width );
    actualExtent.height =
      std::clamp( actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height );

    return actualExtent;
  }
}

auto VulkanSwapchain::chooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes )
  -> VkPresentModeKHR
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

auto VulkanSwapchain::chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats )
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

  return availableFormats[0];
}
