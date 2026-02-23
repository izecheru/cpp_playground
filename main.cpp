#include <iostream>
#include "vulkan_backend.hpp"

int main()
{
  try
  {
    VulkanBase base{};
    base.run();
  }
  catch ( std::exception& e )
  {
    std::cout << "something went wrong" << e.what();
  }
  return 0;
}