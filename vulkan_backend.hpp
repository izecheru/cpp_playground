#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <assert.h>
#include <iostream>
#include <vulkan/vulkan.hpp>

class VulkanBase
{
public:
  explicit VulkanBase( SDL_Window* wnd )
      : m_window{ wnd }
  {
    createInstance( wnd );

    // create the surface
    SDL_Vulkan_CreateSurface( wnd, m_vkInstance, &m_vkSurface );

    populatePhysicalDevices();

    uint32_t graphicsQueueIndex = 0;
    uint32_t presentQueueIndex = 0;
    VkBool32 support;
    uint32_t i = 0;
    for ( auto& queueFamily : m_queueFamilies )
    {
      if ( graphicsQueueIndex == UINT32_MAX && queueFamily.queueCount > 0 &&
           queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
      {
        graphicsQueueIndex = i;
      }

      if ( presentQueueIndex == UINT32_MAX )
      {
        vkGetPhysicalDeviceSurfaceSupportKHR( m_physicalDevices.at( graphicsQueueIndex ), i, m_vkSurface, &support );
        if ( support )
          presentQueueIndex = i;
      }
      ++i;
    }

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo = {
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, // sType
      nullptr,                                    // pNext
      0,                                          // flags
      graphicsQueueIndex,                         // graphicsQueueIndex
      1,                                          // queueCount
      &queuePriority,                             // pQueuePriorities
    };

    VkPhysicalDeviceFeatures deviceFeatures = {};
    const char* deviceExtensionNames[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, // sType
      nullptr,                              // pNext
      0,                                    // flags
      1,                                    // queueCreateInfoCount
      &queueInfo,                           // pQueueCreateInfos
      0,                                    // enabledLayerCount
      nullptr,                              // ppEnabledLayerNames
      1,                                    // enabledExtensionCount
      deviceExtensionNames,                 // ppEnabledExtensionNames
      &deviceFeatures,                      // pEnabledFeatures
    };

    vkCreateDevice( m_physicalDevices.at( graphicsQueueIndex ), &createInfo, nullptr, &m_vkDevice );
    vkGetDeviceQueue( m_vkDevice, graphicsQueueIndex, 0, &m_vkGraphicsQueue );
    vkGetDeviceQueue( m_vkDevice, presentQueueIndex, 0, &m_vkPresentQueue );
  }

  ~VulkanBase()
  {
    vkDestroyDevice( m_vkDevice, nullptr );
    vkDestroyInstance( m_vkInstance, nullptr );
    SDL_DestroyWindow( m_window );
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
  }

  void createInstance( SDL_Window* wnd )
  {
    constexpr VkApplicationInfo appInfo{ .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                         .pApplicationName = "Hello Triangle",
                                         .applicationVersion = VK_MAKE_VERSION( 1, 0, 0 ),
                                         .pEngineName = "No Engine",
                                         .engineVersion = VK_MAKE_VERSION( 1, 0, 0 ),
                                         .apiVersion = vk::ApiVersion14 };

    auto sdlExtCount = 0u;
    SDL_Vulkan_GetInstanceExtensions( wnd, &sdlExtCount, nullptr );
    std::vector<const char*> ext( sdlExtCount );
    SDL_Vulkan_GetInstanceExtensions( wnd, &sdlExtCount, ext.data() );

    VkInstanceCreateInfo instanceInfo{
      .pApplicationInfo = &appInfo, .enabledExtensionCount = sdlExtCount, .ppEnabledExtensionNames = ext.data() };

    vkCreateInstance( &instanceInfo, nullptr, &m_vkInstance );
  }

  // enumerate all the graphics cards and return the vector
  void populatePhysicalDevices()
  {
    uint32_t physicalDeviceCount;
    vkEnumeratePhysicalDevices( m_vkInstance, &physicalDeviceCount, nullptr );
    m_physicalDevices.resize( physicalDeviceCount );
    vkEnumeratePhysicalDevices( m_vkInstance, &physicalDeviceCount, m_physicalDevices.data() );

    if ( m_physicalDevices.empty() )
    {
      throw std::runtime_error( "no graphics cards available" );
    }
  }

  void populateQueueFamily( uint32_t index )
  {
    uint32_t queueFamilyCount{ 0 };
    vkGetPhysicalDeviceQueueFamilyProperties( m_physicalDevices.at( index ), &queueFamilyCount, nullptr );
    m_queueFamilies.resize( queueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties(
      m_physicalDevices.at( index ), &queueFamilyCount, m_queueFamilies.data() );
  }

  void createDevice()
  {
    // auto createInfo = VkDeviceCreateInfo{ .};
    // vkCreateDevice( m_physicalDevices.at( 0 ), );
  }

private:
  SDL_Window* m_window = nullptr;
  // everything is built on top of instance, this is the gateway to vulkan
  VkInstance m_vkInstance;

  // this is an interface to the physical device
  VkDevice m_vkDevice;
  VkQueue m_vkGraphicsQueue;
  VkQueue m_vkPresentQueue;

  VkSurfaceKHR m_vkSurface;

  // a physical device represents a sole complete implementation of Vulkan available to the host, of which there are
  // finite number
  std::vector<VkPhysicalDevice> m_physicalDevices;

  // the work is not uploaded to a device but rather to one of those queues
  std::vector<VkQueueFamilyProperties> m_queueFamilies;
};