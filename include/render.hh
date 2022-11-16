#pragma once

#include <vulkan/vulkan.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <optional>
#include <vector>
#include <set>
#include <limits>

#include "utils.hh"

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

  vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
  vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
  vk::Extent2D chooseSwapExtent(GLFWwindow* window, const vk::SurfaceCapabilitiesKHR& capabilities);
};

class BaseRender {
public:
  BaseRender();
  ~BaseRender();
  virtual void connectWindow(GLFWwindow* w);
  virtual void init();
  virtual void drawFrame();
  virtual void cleanup();
  void setFramebufferResized(bool v);
private:
  bool framebufferResized = false;
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
  vk::Pipeline graphicsPipeline;

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
  virtual void allocateCommandBuffers();
  virtual void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t iamgeIndex);
  virtual void createSyncObjects();

  virtual void cleanupSwapChain();
  virtual void recreateSwapChain();
};
