#pragma once

class GLFWwindow;

class Renderer;
class VulkanInstance;
class RenderAssets;

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
    GLFWwindow* _window;
    Renderer* _renderer;
    VulkanInstance* _vkInstance;
    RenderAssets* _assets;
  };
};
