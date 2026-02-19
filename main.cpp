#include <iostream>
#include "vulkan_device/vulkan_device.hpp"

int main()
{
  try
  {
    const auto& vulkanDevice = VulkanDevice::getInstance();
    vulkanDevice->init();
    vulkanDevice->shutdown();
  }
  catch ( std::exception& e )
  {
    std::cout << "something went wrong" << e.what();
  }
  return 0;
}