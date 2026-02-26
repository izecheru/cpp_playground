#pragma once
#include <array>
#include <glm/glm.hpp>
// #define GLFW_INCLUDE_VULKAN
// #include <GLFW/glfw3.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>

#define MAX_FRAMES_IN_FLIGHT 3

struct UniformBufferoObject
{
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
};

struct Vertex
{
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 texCoord;

  static VkVertexInputBindingDescription getBindingDescription()
  {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof( Vertex );
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
  {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof( Vertex, pos );

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof( Vertex, color );

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof( Vertex, texCoord );

    return attributeDescriptions;
  }
};

const std::vector<Vertex> vertices = { { { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
                                       { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
                                       { { 0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
                                       { { -0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },

                                       { { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
                                       { { 0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
                                       { { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
                                       { { -0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } } };

const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4 };
const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME };

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
  void setupDockspace( ImGuiViewport* viewport );
  void imguiBegin();
  void initImgui();
  void recreateSwapchain();

  void createVertexBuffer();
  void createIndexBuffer();

  // generic buffer funcs
  auto beginSingleTimeCommands() -> VkCommandBuffer;
  void endSingleTimeCommands( VkCommandBuffer commandBuffer );
  //_______

  // images
  void transitionImageLayout( VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout );
  void copyBufferToImage( VkBuffer buffer, VkImage image, uint32_t width, uint32_t height );
  void createTextureImageView();
  void createTextureImage();
  void createTextureSamplers();
  //___

  void updateUniformBuffer( uint32_t imageIndex );

  void createDescriptorSetLayout();
  void createDescriptorPool();
  void createDescriptorSets();

  void cleanSwapchain();
  void createSurface();
  void createSyncObj();
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
  void createDepthResources();

  void createBuffer( VkDeviceSize size,
                     VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags properties,
                     VkBuffer& buffer,
                     VkDeviceMemory& bufferMemory );

  void copyBuffer( VkBuffer src, VkBuffer dst, VkDeviceSize size );

  void run();
  void frameRender( ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data );
  void framePresent( ImGui_ImplVulkanH_Window* wd );
  void initVulkan();

  void createUniformBuffers();

  void recordCommandBuffer( VkCommandBuffer& commandBuffer, uint32_t imageIndex );
  void createCommandBuffer();
  void createCommandPool();

  void createGraphicsPipeline();
  auto createShaderModule( const std::vector<char>& code ) const -> VkShaderModule;

  void createImguiPipeline();

  void createInstance();
  void initWindow();
  void drawFrame();
  void mainLoop();

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

  void createImage( uint32_t width,
                    uint32_t height,
                    VkFormat format,
                    VkImageTiling tiling,
                    VkImageUsageFlags usage,
                    VkMemoryPropertyFlags properties,
                    VkImage& image,
                    VkDeviceMemory& imageMemory );

  auto findMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties ) -> uint32_t;

  auto findSupportedFormat( const std::vector<VkFormat>& candidates,
                            VkImageTiling tiling,
                            VkFormatFeatureFlags features ) -> VkFormat;

  auto findDepthFormat() -> VkFormat;

  auto createImageView( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags ) -> VkImageView;

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
  SDL_Window* window;
  VkInstance m_instance;
  VkDebugUtilsMessengerEXT debugMessenger;

  VkPhysicalDevice m_physicalDevice{ VK_NULL_HANDLE };
  VkPhysicalDeviceProperties m_physicalDeviceProps;
  VkPhysicalDeviceMemoryProperties m_physicalDeviceMemoryProps;
  VkPhysicalDeviceFeatures m_physicalDeviceFeatures;
  VkDevice m_device;

  VkQueue m_graphicsQueue;
  VkQueue m_presentQueue;

  // wd surface
  VkSurfaceKHR m_surface;

  VkSwapchainKHR m_swapChain;
  std::vector<VkImage> m_swapChainImages;

  VkImage m_depthImage;
  VkImageView m_depthView;
  VkDeviceMemory m_depthMemory;

  VkFormat m_swapChainImageFormat;
  VkExtent2D m_swapChainExtent;
  std::vector<VkImageView> m_swapChainImageViews;
  VkDescriptorSetLayout m_descriptorSetLayout;

  VkPipeline m_graphicsPipeline;
  VkPipelineLayout m_pipelineLayout;

  VkPipeline m_imguiPipeline;
  VkPipelineLayout m_imguiPipelineLayout;

  VkCommandPool m_commandPool;
  std::vector<VkCommandBuffer> m_commandBuffers;

  // sync
  std::vector<VkSemaphore> m_imageAvailableSemaphores;
  std::vector<VkSemaphore> m_renderingFinishedSemaphores;
  std::vector<VkFence> m_inFlightFences;
  std::vector<VkFence> m_imagesInFlight;

  VkBuffer m_vertexBuffer;
  VkDeviceMemory m_vertexBufferMemory;

  VkBuffer m_indicesBuffer;
  VkDeviceMemory m_indicesBufferMemory;

  VkBuffer m_stageBuffer;
  VkDeviceMemory m_stageBufferMemory;
  VkImage m_textureImage;
  VkDeviceMemory m_textureImageMemory;
  VkImageView m_textureView;
  VkSampler m_textureSampler;

  std::vector<VkBuffer> m_uniformBuffers;
  std::vector<VkDeviceMemory> m_uniformBuffersMemory;
  std::vector<void*> m_uniformBuffersMapped;

  VkDescriptorPool m_descriptorPool;
  VkDescriptorPool m_imguiPool;
  std::vector<VkDescriptorSet> m_descriptorSets;
  uint32_t m_currentFrame{ 0u };

  bool m_running{ true };
};