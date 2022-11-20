#pragma once

#include <optional>

#include <vulkan/vulkan.hpp>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

struct GLFWwindow;

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

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;

  static vk::VertexInputBindingDescription getBindingDescription();
  static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions();
};

struct UniformBufferObj {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
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

  uint32_t findMemType(uint32_t typeFilter, vk::MemoryPropertyFlags prop, const vk::PhysicalDevice& device);
  
  std::tuple<vk::Buffer, vk::DeviceMemory> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memProp, vk::Device device, vk::PhysicalDevice physicalDevice);
  void copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size, vk::CommandPool commandPool, vk::Device device, vk::Queue queue);
};

class BaseRenderer {
public:
  BaseRenderer();
  virtual ~BaseRenderer();
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

  vk::DescriptorSetLayout descriptorSetLayout;
  vk::PipelineLayout pipelineLayout;
  vk::Pipeline graphicsPipeline;

  vk::Buffer vertexBuffer;
  vk::DeviceMemory vertexBufferMem;

  vk::Buffer indexBuffer;
  vk::DeviceMemory indexBufferMem;

  vk::CommandPool commandPool;
  std::vector<vk::CommandBuffer> commandBuffers;
  std::vector<vk::Semaphore> imageAvailableSemaphores;
  std::vector<vk::Semaphore> renderFinishedSemaphores;
  std::vector<vk::Fence> inFlightFences;

  std::vector<vk::Buffer> uniformBuffers;
  std::vector<vk::DeviceMemory> uniformMems;
  std::vector<void*> uniformBuffersMapped;

  vk::DescriptorPool descPool;
  std::vector<vk::DescriptorSet> descriptorSets;
private:
  virtual void createInstance();
  virtual void createSurface();
  virtual void pickPhysicalDevice();
  virtual void createLogicalDevice();
  virtual void createSwapChain();
  virtual void createImageViews();
  virtual void createRenderPass();
  virtual void createDescriptorSetLayout();
  virtual void createGraphicsPipeline();
  virtual void createFrameBuffers();
  virtual void createCommandPool();
  virtual void allocateCommandBuffers();
  virtual void allocateVertexBuffer();
  virtual void allocateIndexBuffer();
  virtual void allocateUniformBuffers();
  virtual void createDescriptorPool();
  virtual void createDescriptorSets();
  virtual void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t iamgeIndex, uint32_t currentFrame);
  virtual void createSyncObjects();

  virtual void cleanupSwapChain();
  virtual void recreateSwapChain();

  virtual void updateUniformBuffer(uint32_t currentFrame);
};
