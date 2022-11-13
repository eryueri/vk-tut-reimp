#include "render.hh"

static const uint32_t WIDTH = 800;
static const uint32_t HEIGHT = 600;

namespace HelloTriangle {
  Render* Render::_instance = nullptr;
  Render::Render() {}
  Render::~Render() {}

  Render* Render::getInstance() {
    if (!_instance) {
      _instance = new Render;
    }
    return _instance;
  }

  void Render::run() {
    initWindow();
    mainLoop();
    cleanup();
  }

  void Render::initWindow() {
    std::cerr << "initializing window...\n";
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(WIDTH, HEIGHT, "vulkan-reimp", nullptr, nullptr);
    glfwSetKeyCallback();
  }

  void Render::mainLoop() {
    while(!glfwWindowShouldClose(window)) {
      glfwPollEvents();
    }
  }

  void Render::cleanup() {
    std::cerr << "cleaning up...\n";
    glfwDestroyWindow(window);
    glfwTerminate();
  }
};
