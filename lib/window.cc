#include "window.hh"

static const uint32_t WIDTH = 800;
static const uint32_t HEIGHT = 600;

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

    window = glfwCreateWindow(WIDTH, HEIGHT, "vulkan-reimp", nullptr, nullptr);
    render.connectWindow(window);
    render.init();
    
    glfwSetWindowUserPointer(window, &render);

    glfwSetKeyCallback(window, keyCallBack);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallBack);
  }
  void MainWindow::mainLoop() {
    while(!glfwWindowShouldClose(window)) {
      glfwPollEvents();
      render.drawFrame();
    }
  }
  void MainWindow::cleanup() {
    render.cleanup();
    glfwDestroyWindow(window);
    glfwTerminate();
  }
  void MainWindow::keyCallBack(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
  }
  void MainWindow::framebufferResizeCallBack(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<BaseRender*>(glfwGetWindowUserPointer(window));
    app->setFramebufferResized(true);
  }
};
