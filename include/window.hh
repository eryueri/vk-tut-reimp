#pragma once

#include "render.hh"

namespace Window{
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
  private:
    static MainWindow* _instance;
    GLFWwindow* window;
  };
};
