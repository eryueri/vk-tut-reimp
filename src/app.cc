#include "app.hh"

#include <iostream>
#include <time.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "VulkanInstance.hh"
#include "RenderAssets.hh"
#include "VertexRenderer.hh"

#define WIDTH (uint32_t)800
#define HEIGHT (uint32_t)600

namespace myWindow {
  MainWindow* MainWindow::_instance = nullptr;
  MainWindow::MainWindow() {}
  MainWindow::~MainWindow() {}
  MainWindow* MainWindow::getInstance() {
    if (!_instance) {
      _instance = new MainWindow;
    }
    return _instance;
  }
  void MainWindow::run() {
    init();
    mainLoop();
    cleanup();
  }
  void MainWindow::init() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    _window = glfwCreateWindow(WIDTH, HEIGHT, "vulkan-reimp", nullptr, nullptr);

    _vkInstance = new VulkanInstance;
    _assets = new RenderAssets;
    _renderer = new Renderer;

    _vkInstance->setWindow(_window);
    _vkInstance->init();
    _assets->init(_vkInstance);
    _renderer->init(_vkInstance, _assets);
    
    glfwSetWindowUserPointer(_window, &_vkInstance);

    glfwSetKeyCallback(_window, keyCallBack);
    glfwSetFramebufferSizeCallback(_window, framebufferResizeCallBack);
  }
  void MainWindow::mainLoop() {
    while(!glfwWindowShouldClose(_window)) {
      glfwPollEvents();
      _renderer->drawFrame();
    }
  }
  void MainWindow::cleanup() {
    _assets->cleanup();
    _vkInstance->cleanup();
    glfwDestroyWindow(_window);
    glfwTerminate();
  }
  void MainWindow::keyCallBack(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
  }
  void MainWindow::framebufferResizeCallBack(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<VulkanInstance*>(glfwGetWindowUserPointer(window));
    app->setFrameBufferResized(true);
  }
}
