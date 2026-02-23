#define SDL_MAIN_HANDLED
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
    std::cout << "something went wrong: exception-> " << e.what();
  }
  return 0;
}