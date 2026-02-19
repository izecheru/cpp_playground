#pragma once
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

struct GLFWwindow;

inline const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
inline const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct VulkanPlatform
{
  VkInstance instance = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
};

struct SwapChainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices
{
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() const
  {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct GPUQueue
{
  VkQueue handle{ VK_NULL_HANDLE };
  uint32_t familyIndex = UINT32_MAX;
};

class VulkanDevice
{
public:
  void init();
  void setupDebug();
  void createWindow();
  void createInstance();
  void createWindowSurface();
  void pickPhysicalDevice();
  void createLogicalDevice();
  void shutdown();

  static auto getGraphicsQueue() -> GPUQueue&;
  static auto getPresentQueue() -> GPUQueue&;
  static auto getGlfwWindow() -> GLFWwindow*;

  static auto getPhysicalDevice() -> VkPhysicalDevice&;
  static auto getLogicalDevice() -> VkDevice&;
  static auto getSurface() -> VkSurfaceKHR&;

  inline static auto getInstance() -> VulkanDevice*
  {
    static VulkanDevice instance;
    return &instance;
  }

private:
  auto querySwapChainSupport( VkPhysicalDevice& device ) -> SwapChainSupportDetails;
  auto findQueueFamilies( VkPhysicalDevice& device ) -> QueueFamilyIndices;
  bool isDeviceSuitable( VkPhysicalDevice& device );

  VulkanDevice() = default;
  ~VulkanDevice() = default;
  VulkanDevice( const VulkanDevice& ) = delete;
  VulkanDevice& operator=( const VulkanDevice& ) = delete;

private:
  static inline VulkanPlatform m_platform;
  static inline GLFWwindow* m_window{ nullptr };

  VkDebugUtilsMessengerEXT m_debugMessenger{ VK_NULL_HANDLE };

  static inline GPUQueue m_presentQueue;
  static inline GPUQueue m_graphicsQueue;
};