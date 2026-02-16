#include "vulkan_backend.hpp"

int main()
{
  try
  {
    HelloTriangleApplication app;
    app.run();
  }
  catch ( std::exception& e )
  {
    std::cout << e.what();
  }
  return 0;
}