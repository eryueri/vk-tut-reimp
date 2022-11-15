#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <optional>
#include <set>

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete();
};

struct SwapChainSupportDetails {
  vk::SurfaceCapabilitiesKHR capabilities;
  std::vector<vk::SurfaceFormatKHR> formats;
  std::vector<vk::PresentModeKHR> presentModes;
};

namespace HelperFunc {
  bool validationLayerSupportChecked(const std::vector<const char*> validationLayers);
  bool deviceExtensionSupportChecked(vk::PhysicalDevice device, const std::vector<const char*> deviceExtension);

  QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface);
  SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface);

  bool isDeviceSuitable(vk::PhysicalDevice phyDevice, vk::SurfaceKHR surface);

  vk::ShaderModule createShaderModule(vk::Device device, const std::vector<char>& code);
};

class BaseRender {
public:
  BaseRender();
  ~BaseRender();
  virtual void connectWindow(GLFWwindow* w);
  virtual void init();
  virtual void drawFrame();
  virtual void cleanup();
private:
  GLFWwindow* window;
private:
  vk::Instance instance;
  vk::SurfaceKHR surface;
  vk::PhysicalDevice physicalDevice;
  vk::Device device;
  vk::Queue graphicQueue;
  vk::Queue presentQueue;

  vk::SwapchainKHR swapChain;
  vk::Format swapChainImageFormat;
  vk::Extent2D swapChainExtent;

  std::vector<vk::Image> swapChainImages;
  std::vector<vk::ImageView> swapChainImageViews;
  std::vector<vk::Framebuffer> swapChainFramebuffers;

  vk::RenderPass renderPass;
  vk::PipelineLayout pipelineLayout;
  vk::Pipeline graphicPipeline;

  vk::CommandPool commandPool;
  std::vector<vk::CommandBuffer> commandBuffers;
  std::vector<vk::Semaphore> imageAvailbleSemaphores;
  std::vector<vk::Semaphore> renderFinishedSemaphores;
  std::vector<vk::Fence> inFlightFences;
private:
  virtual void createInstance();
  virtual void createSurface();
  virtual void pickPhysicalDevice();
  virtual void createLogicalDevice();
  virtual void createSwapChain();
  virtual void createImageViews();
  virtual void createRenderPass();
  virtual void createGraphicsPipeline();
  virtual void createFrameBuffers();
  virtual void createCommandPool();
  virtual void createCommandBuffer();
  virtual void createSyncObjects();
};
