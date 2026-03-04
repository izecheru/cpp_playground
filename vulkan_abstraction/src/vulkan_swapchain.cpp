#include "vulkan_abstraction/vulkan_swapchain.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <algorithm>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include "vulkan_abstraction/vulkan_device.hpp"

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

VulkanSwapchain::VulkanSwapchain( VulkanDevice* vulkanDevice, SDL_Window* wnd )
    : m_pDevice{ vulkanDevice }
    , m_window{ wnd }
{
  createSwapchain();
  createImageViews();
  createSyncObjects();
}

VulkanSwapchain::~VulkanSwapchain()
{
  destroy();
}

void VulkanSwapchain::destroy()
{
  vkDeviceWaitIdle( m_pDevice->getLogicalDevice() );

  for ( auto& entry : m_swapchainImages )
  {
    vkDestroySemaphore( m_pDevice->getLogicalDevice(), entry.imageAvailable, nullptr );
    vkDestroySemaphore( m_pDevice->getLogicalDevice(), entry.renderingFinished, nullptr );
    vkDestroyFence( m_pDevice->getLogicalDevice(), entry.inFlight, nullptr );
  }

  for ( auto& swapchainImage : m_swapchainImages )
  {
    vkDestroyImageView( m_pDevice->getLogicalDevice(), swapchainImage.imageView, nullptr );
  }

  vkDestroySwapchainKHR( m_pDevice->getLogicalDevice(), m_swapchain, nullptr );
}

void VulkanSwapchain::createSwapchain()
{
  SwapchainSupportDetails swapchainSupport = querySwapchainSupport();
  auto surfaceFormat = chooseSwapSurfaceFormat( swapchainSupport.formats );
  auto presentMode = chooseSwapPresentMode( swapchainSupport.presentModes );
  auto extent = chooseSwapExtent( swapchainSupport.capabilities );
  m_imageCount = swapchainSupport.capabilities.minImageCount + 1;

  if ( swapchainSupport.capabilities.maxImageCount > 0 && m_imageCount > swapchainSupport.capabilities.maxImageCount )
  {
    m_imageCount = swapchainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = m_pDevice->getSurface();

  createInfo.minImageCount = m_imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  // we need VK_IMAGE_USAGE_TRANSFER_DST_BIT if we want to do post processing as this is a color attachment
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32_t queueFamilyIndices[] = { m_pDevice->getGraphicsQueue().familyIndex,
                                    m_pDevice->getPresentQueue().familyIndex };

  if ( queueFamilyIndices[0] != queueFamilyIndices[1] )
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

  createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if ( vkCreateSwapchainKHR( m_pDevice->getLogicalDevice(), &createInfo, nullptr, &m_swapchain ) != VK_SUCCESS )
  {
    throw std::runtime_error( "failed to create swap chain!" );
  }

  vkGetSwapchainImagesKHR( m_pDevice->getLogicalDevice(), m_swapchain, &m_imageCount, nullptr );
  std::vector<VkImage> images( m_imageCount );
  m_swapchainImages.resize( m_imageCount );
  vkGetSwapchainImagesKHR( m_pDevice->getLogicalDevice(), m_swapchain, &m_imageCount, images.data() );
  for ( auto i = 0u; i < m_imageCount; i++ )
  {
    m_swapchainImages.at( i ).image = images.at( i );
  }

  m_swapchainFormat = surfaceFormat.format;
  m_swapchainExtent = extent;
}

void VulkanSwapchain::createImageViews()
{
  for ( size_t i = 0; i < m_imageCount; i++ )
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

    if ( vkCreateImageView(
           m_pDevice->getLogicalDevice(), &createInfo, nullptr, &m_swapchainImages.at( i ).imageView ) != VK_SUCCESS )
    {
      throw std::runtime_error( "failed to create image views!" );
    }
  }
}

void VulkanSwapchain::createSyncObjects()
{
  for ( auto i = 0u; i < m_imageCount; i++ )
  {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if ( vkCreateSemaphore(
           m_pDevice->getLogicalDevice(), &semaphoreInfo, nullptr, &m_swapchainImages.at( i ).imageAvailable ) !=
           VK_SUCCESS ||
         vkCreateSemaphore(
           m_pDevice->getLogicalDevice(), &semaphoreInfo, nullptr, &m_swapchainImages.at( i ).renderingFinished ) !=
           VK_SUCCESS ||
         vkCreateFence( m_pDevice->getLogicalDevice(), &fenceInfo, nullptr, &m_swapchainImages.at( i ).inFlight ) !=
           VK_SUCCESS )
    {
      throw std::runtime_error( "failed to create sempahores/ fences" );
    }
  }
}

auto VulkanSwapchain::getCurrentSwapchainImage() -> SwapchainImage&
{
  return m_swapchainImages.at( m_currentFrame );
}

auto VulkanSwapchain::getCurrentFrame() -> uint32_t
{
  return m_currentFrame;
}

auto VulkanSwapchain::getSwapchainImageFormat() -> VkFormat&
{
  return m_swapchainFormat;
}

auto VulkanSwapchain::getSwapchain() -> VkSwapchainKHR&
{
  return m_swapchain;
}

auto VulkanSwapchain::getNextImageIndex() -> uint32_t
{
  uint32_t imageIndex{ 0u };

  vkAcquireNextImageKHR( m_pDevice->getLogicalDevice(),
                         m_swapchain,
                         UINT64_MAX,
                         m_swapchainImages.at( m_currentFrame ).imageAvailable,
                         VK_NULL_HANDLE,
                         &imageIndex );
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
    SDL_GetWindowSize( m_window, &width, &height );

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
