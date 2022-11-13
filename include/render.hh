#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <optional>
#include <vector>
#include <set>

namespace HelloTriangle {
  class Render {
  public:
    static Render* getInstance();
    void run();
  private:
    Render();
    ~Render();
    void initWindow();
    void mainLoop();
    void cleanup();
  private:
    static Render* _instance;
    GLFWwindow* window;
  };
};
