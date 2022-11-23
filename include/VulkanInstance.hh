#include <memory>
#include <vulkan/vulkan.hpp>

struct QueueFamilyIndices;

class GLFWwindow;

class VulkanInstance{
public:
  VulkanInstance() = default;
  ~VulkanInstance();
  void init();
  void cleanup();
  void currentFrameInc();
  void setWindow(GLFWwindow* w);

  void setFrameBufferResized(bool v);
  void recreateSwapChain();
public:
  uint32_t acquireImage();
public:
  uint32_t getCurrentFrame() const;
  vk::PhysicalDevice getGPU() const;
  vk::Device getLogicalDevice() const;
  vk::Queue getGraphicsQueue() const;
  vk::Queue getPresentQueue() const;
  vk::Extent2D getSwapChainExtent() const;
  vk::Framebuffer getFramebuffer(uint32_t imageIndex) const;
  vk::RenderPass getRenderPass() const;
  vk::CommandPool getCommandPool() const;
  vk::CommandBuffer getCommandBuffer() const;
  vk::Semaphore getImageSemaphore(uint32_t index) const;
  vk::Semaphore getRenderSemaphore(uint32_t index) const;
  vk::Fence getFence(uint32_t index) const;
public:
  void waitForFence() const;
  vk::CommandBuffer getCommandBufferBegin() const;
  void getCommandBufferEnd() const;
  void applyGraphicsQueue() const;
  void applyPresentQueue(uint32_t imageIndex);
private:
  GLFWwindow* _window = nullptr;
private:
  bool _enableValidationLayers = false;
  bool _frameBufferResized = false;
  uint32_t _currentFrame = 0;
  QueueFamilyIndices* _queueIndices = nullptr;
private:
  vk::Instance _instance = nullptr;
  vk::SurfaceKHR _surface = nullptr;
  vk::PhysicalDevice _gpu = nullptr;
  vk::Device _device = nullptr;
  vk::Queue _graphicsQueue = nullptr;
  vk::Queue _presentQueue = nullptr;

  vk::SwapchainKHR _swapChain = nullptr;
  vk::Format _swapChainImageFormat;
  vk::Extent2D _swapChainExtent;

  std::vector<vk::Image> _swapChainImages;
  std::vector<vk::ImageView> _swapChainImageViews;
  std::vector<vk::Framebuffer> _swapChainFramebuffers;

  vk::RenderPass _renderPass;

  vk::DescriptorSetLayout _descriptorSetLayout = nullptr;

  vk::CommandPool _commandPool = nullptr;
  std::vector<vk::CommandBuffer> _commandBuffers;
  std::vector<vk::Semaphore> _imageAvailableSemaphores;
  std::vector<vk::Semaphore> _renderFinishedSemaphores;
  std::vector<vk::Fence> _inFlightFences;

  std::vector<vk::Buffer> _uniformBuffers;
  std::vector<vk::DeviceMemory> _uniformMems;
  std::vector<void*> _uniformBuffersMapped;

  vk::DescriptorPool _descPool = nullptr;
  std::vector<vk::DescriptorSet> _descriptorSets;
private:
  void createInstance();
  void createSurface();
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createSwapChain();
  void createImageViews();
  void createRenderPass();
  void createFrameBuffers();
  void createCommandPool();
  void allocateCommandBuffers();
  void createSyncObjects();
private:
  void cleanupSwapChain();
  void cleanupSyncObjects();
  void cleanupCommandPool();
  void cleanupLogicalDevice();
  void cleanupSurface();
  void cleanupInstance();
};
