#pragma once
#include <vector>
#include <vulkan/vulkan.h>

struct VulkanDevice;
struct QueueFamilyIndices;

struct SwapchainImage
{
  VkImage image{ VK_NULL_HANDLE };
  VkImageView imageView{ VK_NULL_HANDLE };
};

struct SwapchainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

class VulkanSwapchain
{
public:
  explicit VulkanSwapchain( std::shared_ptr<VulkanDevice> device );
  ~VulkanSwapchain();

  void destroy();
  void createSwapchain();
  void createImageViews();
  void createSyncObjects();
  // void createFramebuffers();
  //  void createRenderPass();
  auto getImageView( uint32_t index ) -> VkImageView;
  auto getNextImageIndex() -> uint32_t;

private:
  auto chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities ) -> VkExtent2D;
  auto chooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes ) -> VkPresentModeKHR;
  auto chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats ) -> VkSurfaceFormatKHR;
  auto querySwapchainSupport() -> SwapchainSupportDetails;

private:
  bool m_destroyed{ false };
  VulkanDevice* m_pDevice;
  VkSwapchainKHR m_swapchain;

  VkRenderPass m_renderPass;
  std::vector<VkFramebuffer> m_framebuffers;

  std::vector<SwapchainImage> m_swapchainImages;
  std::vector<SwapchainImage> m_swapchainDepthImages;

  std::vector<VkRenderingAttachmentInfo> m_attachments;

  VkFormat m_swapchainFormat;
  VkExtent2D m_swapchainExtent;

  uint32_t m_width;
  uint32_t m_height;

  VkSemaphore m_imageAvailableSemaphore;
  VkSemaphore m_renderFinishedSemaphore;
  VkFence m_inFlightFence;
};