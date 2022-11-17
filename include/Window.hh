#pragma once

#include "BaseRenderer.hh"

namespace myWindow{
  class MainWindow{
  public:
    static MainWindow* getInstance();
    void run();
  private:
    MainWindow();
    ~MainWindow();
    void init();
    void mainLoop();
    void cleanup();
    static void keyCallBack(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void framebufferResizeCallBack(GLFWwindow* window, int width, int height);
  private:
    static MainWindow* _instance;
    GLFWwindow* window;
    BaseRenderer render;
  };
};
