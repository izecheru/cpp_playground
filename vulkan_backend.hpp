#pragma once
#include <cstdlib>
#include <GLFW/glfw3.h>
#include <assert.h>
#include <iostream>
#include <vulkan/vulkan.hpp>

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct VkInitData
{
  // this is used for debugging setup
  std::vector<VkLayerProperties> availableLayers;
  // extensions supported
  std::vector<VkExtensionProperties> extensionsProperties;

  std::vector<const char*> extenions;
  // we need this to enumerate all rendering capable devices
  std::vector<VkPhysicalDevice> devices;

  // this is just a helper function to fill the vectors above
  // the one with devices i cannot fill in this function because i need
  // the instance to enumerate devices and i create that instance later than this func call
  void fill()
  {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, nullptr );
    extensionsProperties.resize( extensionCount );
    vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, extensionsProperties.data() );

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties( &layerCount, nullptr );
    availableLayers.resize( layerCount );
    vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

    extenions = std::vector<const char*>( glfwExtensions, glfwExtensions + glfwExtensionCount );

    if ( enableValidationLayers )
    {
      extenions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
    }
  }
};

class HelloTriangleApplication
{
public:
  void run()
  {
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  /**
   * @brief This is the first func to be called in the vulkan initialization chain
   */
  void createInstance()
  {
    m_data.fill();

    if ( enableValidationLayers && !checkValidationLayerSupport() )
    {
      throw std::runtime_error( "validation layers requested, but not available!" );
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.pEngineName = "test";
    appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.apiVersion = VK_API_VERSION_1_4;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    if ( enableValidationLayers )
    {
      createInfo.enabledLayerCount = static_cast<uint32_t>( validationLayers.size() );
      createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
      createInfo.enabledLayerCount = 0;
    }

    createInfo.enabledExtensionCount = m_data.extenions.size();
    createInfo.ppEnabledExtensionNames = m_data.extenions.data();
    if ( vkCreateInstance( &createInfo, nullptr, &m_vkInstance ) != VK_SUCCESS )
    {
      throw std::runtime_error( "failed to create instance!" );
    }

    // now after instance creation, iterate over devices
    uint32_t devCount;
    vkEnumeratePhysicalDevices( m_vkInstance, &devCount, nullptr );
    assert( devCount != 0 && "something went wrong, no device available" );
    vkEnumeratePhysicalDevices( m_vkInstance, &devCount, m_data.devices.data() );
  }

  void initVulkan()
  {
    glfwInit();

    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

    // window should be a standalone lib or dll
    m_window = glfwCreateWindow( 800, 800, "Vulkan", nullptr, nullptr );

    createInstance();
    setupDebugCallback();
    pickPhisycalDevice();
  }

  void setupDebugCallback()
  {
    if ( !enableValidationLayers )
      return;
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr; // Optional
    if ( createDebugUtilsMessengerEXT( m_vkInstance, &createInfo, nullptr, &debugMessenger ) != VK_SUCCESS )
    {
      throw std::runtime_error( "could not set up debug log" );
    }
  }

  void destroyDebugUtilsMessengerEXT( VkInstance instance,
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

  /**
   * @brief This is needed to load the vkCreateDebugUtilsMessengerEXT with GetInstanceProcAddr
   * @param instance
   * @param pCreateInfo
   * @param pAllocator
   * @param pDebugMessenger
   * @return
   */
  VkResult createDebugUtilsMessengerEXT( VkInstance instance,
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

  void mainLoop()
  {
    while ( !glfwWindowShouldClose( m_window ) )
    {
      glfwPollEvents();
    }
  }

  /**
   * @brief Checks for validation layer support and populates the vector
   * @return
   */
  bool checkValidationLayerSupport()
  {
    for ( const char* layerName : validationLayers )
    {
      bool layerFound = false;

      for ( const auto& layerProperties : m_data.availableLayers )
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

  void cleanup()
  {
    destroyDebugUtilsMessengerEXT( m_vkInstance, debugMessenger, nullptr );
    vkDestroyInstance( m_vkInstance, nullptr );
    glfwDestroyWindow( m_window );
    glfwTerminate();
  }

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                       VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                       const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                       void* pUserData )
  {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
  }

  void pickPhisycalDevice()
  {
    for ( const auto& device : m_data.devices )
    {
      if ( isDeviceSuitable( device ) )
      {
        m_physicalDevice = device;
        break;
      }
    }

    if ( m_physicalDevice == VK_NULL_HANDLE )
    {
      throw std::runtime_error( "failed to find a suitable GPU!" );
    }
  }

  bool isDeviceSuitable( const VkPhysicalDevice& device )
  {
    return true;
  }

private:
  GLFWwindow* m_window;

  // gateway to vulkan, everything goes through the instance
  VkInstance m_vkInstance;

  // this is literally a graphics capable device, GPU
  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debugMessenger;
  VkInitData m_data;
};
