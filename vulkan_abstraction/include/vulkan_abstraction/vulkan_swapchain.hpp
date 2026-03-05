#pragma once
#include <vector>
#include <vulkan/vulkan.h>

#define MAX_FRAMES_IN_FLIGHT 3

struct VulkanDevice;
struct QueueFamilyIndices;
struct SDL_Window;

struct SwapchainImage
{
  VkImage image{ VK_NULL_HANDLE };
  VkImageView imageView{ VK_NULL_HANDLE };

  VkSemaphore imageAvailable{};
  VkSemaphore renderingFinished{};
  VkFence inFlight{ VK_NULL_HANDLE };
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

  auto getCurrentCommandBuffer() -> VkCommandBuffer&;

  void onUpdate();
  void recreateSwapchain();

  void presentFrame();
  bool beginRendering();
  void endRendering();

  auto getSwapchainFormat() -> VkFormat&;

private:
  void destroy();
  void createSwapchain();
  void createImageViews();
  void createCommandPool();
  void createCommandBuffers();
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
  uint32_t m_imageIndex{ 0u };

  std::vector<VkCommandBuffer> m_commandBuffers;
  VkCommandPool m_commandPool;

  bool m_rendering{ true };
};