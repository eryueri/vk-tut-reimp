#include "window.hh"

static const uint32_t WIDTH = 800;
static const uint32_t HEIGHT = 600;

namespace Window {
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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(WIDTH, HEIGHT, "vulkan-reimp", nullptr, nullptr);
    glfwSetKeyCallback(window, keyCallBack);
  }
  void MainWindow::mainLoop() {
    while(!glfwWindowShouldClose(window)) {
      glfwPollEvents();
    }
  }
  void MainWindow::cleanup() {
    glfwDestroyWindow(window);
    glfwTerminate();
  }
  void MainWindow::keyCallBack(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
  }
};
