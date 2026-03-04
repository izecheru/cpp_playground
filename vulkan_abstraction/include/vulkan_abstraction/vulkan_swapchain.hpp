#pragma once
#include <vector>
#include <vulkan/vulkan.h>

struct VulkanDevice;
struct QueueFamilyIndices;
struct SDL_Window;

struct SwapchainFrame
{
  SwapchainImage image;
  uint32_t index;
};

struct SwapchainImage
{
  VkImage image{ VK_NULL_HANDLE };
  VkImageView imageView{ VK_NULL_HANDLE };

  VkSemaphore imageAvailable{};
  VkSemaphore renderingFinished{};
  VkFence inFlight{};
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
  explicit VulkanSwapchain( VulkanDevice* vulkanDevice, SDL_Window* wnd );
  ~VulkanSwapchain();

  auto getNextImageIndex() -> uint32_t;
  auto getCurrentSwapchainImage() -> SwapchainImage&;
  auto getCurrentFrame() -> uint32_t;
  auto getSwapchainImageFormat() -> VkFormat&;
  auto getSwapchain() -> VkSwapchainKHR&;

private:
  void destroy();
  void createSwapchain();
  void createImageViews();
  void createSyncObjects();
  auto chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities ) -> VkExtent2D;
  auto chooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes ) -> VkPresentModeKHR;
  auto chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats ) -> VkSurfaceFormatKHR;
  auto querySwapchainSupport() -> SwapchainSupportDetails;

private:
  SDL_Window* m_window;
  bool m_destroyed{ false };
  VulkanDevice* m_pDevice;
  VkSwapchainKHR m_swapchain;

  std::vector<SwapchainImage> m_swapchainImages;

  VkFormat m_swapchainFormat;
  VkExtent2D m_swapchainExtent;

  uint32_t m_width;
  uint32_t m_height;
  uint32_t m_imageCount;
  uint32_t m_currentFrame{ 0u };
};