#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.h>

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME
                                                     };

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices
{
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() const
  {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct SwapChainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

class VulkanBase
{
public:
  void createFrameBuffers();
  void createSurface();
  void createSyncObj();

  void createRenderPass();

  void createLogicalDevice();
  void pickPhysicalDevice();
  bool isDeviceSuitable( VkPhysicalDevice& device );
  bool checkDeviceExtensionSupport( VkPhysicalDevice& device );

  auto querySwapChainSupport( VkPhysicalDevice& device ) -> SwapChainSupportDetails;
  auto chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities ) -> VkExtent2D;
  auto chooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes ) -> VkPresentModeKHR;
  auto chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats ) -> VkSurfaceFormatKHR;
  void createSwapChain();
  void createImageViews();

  void run();
  void initVulkan();

  void recordCommandBuffer( VkCommandBuffer& commandBuffer, uint32_t imageIndex );
  void createCommandBuffer();
  void createCommandPool();

  void createGraphicsPipeline();
  auto createShaderModule( const std::vector<char>& code ) const -> VkShaderModule;

  void createInstance();
  void initWindow();
  void drawFrame();
  void mainLoop();

  // this should not take any params in this context since we store the device as a member variable
  // but for a clearer view when we abstract this code i'll let it as is
  auto findQueueFamilies( VkPhysicalDevice& device ) -> QueueFamilyIndices;

  void cleanup();

  void setupDebug();
  bool checkValidationLayerSupport();
  void populateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT& info );

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                       VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                       const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                       void* pUserData )
  {
    /*
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT = 0x00000001,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT = 0x00000010,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT = 0x00000100,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT = 0x00001000,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT = 0x7FFFFFFF
   */
    switch ( messageSeverity )
    {
    case 1 << 0:
    case 1 << 4:
      spdlog::info( "---VK_INFO---\n{}\n---", pCallbackData->pMessage );
      break;
    case 1 << 8:
      spdlog::warn( "---VK_WARN---\n{}\n---", pCallbackData->pMessage );
      break;
    case 1 << 12:
      spdlog::error( "---VK_ERR---\n{}\n---", pCallbackData->pMessage );
      break;
    }

    return VK_FALSE;
  }

  auto getRequiredExtensions() -> std::vector<const char*>;

  void DestroyDebugUtilsMessengerEXT( VkInstance instance,
                                      VkDebugUtilsMessengerEXT debugMessenger,
                                      const VkAllocationCallbacks* pAllocator )
  {
    auto func =
      (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" );
    if ( func != nullptr )
    {
      func( instance, debugMessenger, pAllocator );
    }
  }

  VkResult CreateDebugUtilsMessengerEXT( VkInstance instance,
                                         const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                         const VkAllocationCallbacks* pAllocator,
                                         VkDebugUtilsMessengerEXT* pDebugMessenger )
  {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" );
    if ( func != nullptr )
    {
      return func( instance, pCreateInfo, pAllocator, pDebugMessenger );
    }
    else
    {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
  }

private:
  GLFWwindow* window;
  VkInstance m_instance;
  VkDebugUtilsMessengerEXT debugMessenger;

  VkPhysicalDevice m_physicalDevice{ VK_NULL_HANDLE };
  VkDevice m_device;

  VkQueue m_graphicsQueue;
  VkQueue m_presentQueue;

  // window surface
  VkSurfaceKHR m_surface;

  VkSwapchainKHR m_swapChain;
  std::vector<VkImage> m_swapChainImages;
  std::vector<VkFramebuffer> m_swapChainFramebuffers;
  VkFormat m_swapChainImageFormat;
  VkExtent2D m_swapChainExtent;
  std::vector<VkImageView> m_swapChainImageViews;
  VkPipelineLayout m_pipelineLayout;
  VkPipeline m_graphicsPipeline;
  VkRenderPass m_renderPass;

  VkCommandPool m_commandPool;
  VkCommandBuffer m_commandBuffer;

  // sync
  VkSemaphore m_imageAvailableSemaphore;
  VkSemaphore m_renderFinishedSemaphore;
  VkFence m_inFlightFence;
};