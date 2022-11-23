#include "VulkanInstance.hh"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <set>
#include <limits>
#include <chrono>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "Structs.hh"
#include "VkUtils.hh"
#include "Macros.hh"

const std::vector<const char*> validationLayers = {
  "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

VulkanInstance::~VulkanInstance() {
  delete _queueIndices;
}

void VulkanInstance::init() {
  createInstance();
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();
  createSwapChain();
  createImageViews();
  createRenderPass();
  createFrameBuffers();
  createCommandPool();
  allocateCommandBuffers();
  createSyncObjects();
}

void VulkanInstance::cleanup() {
  cleanupSwapChain();
  cleanupRenderPass();
  cleanupSyncObjects();
  cleanupCommandPool();
  cleanupLogicalDevice();
  cleanupSurface();
  cleanupInstance();
}

void VulkanInstance::currentFrameInc() {
  _currentFrame = ++_currentFrame % MAX_FRAMES_IN_FLIGHT;
}

void VulkanInstance::setWindow(GLFWwindow* w) {
  _window = w;
}

void VulkanInstance::setFrameBufferResized(bool v) {
  _frameBufferResized = v;
}

void VulkanInstance::recreateSwapChain() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(_window, &width, &height);
  while(width == 0 || height == 0) {
    glfwGetFramebufferSize(_window, &width, &height);
    glfwWaitEvents();  
  }

  cleanupSwapChain();

  createSwapChain();
  createImageViews();
  createFrameBuffers();
}

uint32_t VulkanInstance::acquireImage() {
  uint32_t imageIndex;
  auto acquireResult = _device.acquireNextImageKHR(_swapChain, UINT64_MAX, _imageAvailableSemaphores[_currentFrame], nullptr, &imageIndex);
  if (acquireResult == vk::Result::eErrorOutOfDateKHR) {
    recreateSwapChain();
  } else if (acquireResult != vk::Result::eSuccess && acquireResult != vk::Result::eSuboptimalKHR) {
    throw std::runtime_error("trouble acquiring next image");
  }

  return imageIndex;
}

uint32_t VulkanInstance::getCurrentFrame() const {
  return _currentFrame;
}

vk::PhysicalDevice VulkanInstance::getGPU() const {
  return _gpu;
}

vk::Device VulkanInstance::getLogicalDevice() const {
  return _device;
}

vk::Queue VulkanInstance::getGraphicsQueue() const {
  return _graphicsQueue;
}

vk::Queue VulkanInstance::getPresentQueue() const {
  return _presentQueue;
}

vk::Extent2D VulkanInstance::getSwapChainExtent() const {
  return _swapChainExtent;
}

vk::Framebuffer VulkanInstance::getFramebuffer(uint32_t imageIndex) const {
  return _swapChainFramebuffers[imageIndex];
}

vk::RenderPass VulkanInstance::getRenderPass() const {
  return _renderPass;
}

vk::CommandPool VulkanInstance::getCommandPool() const {
  return _commandPool;
}

vk::Semaphore VulkanInstance::getImageSemaphore(uint32_t index) const {
  return _imageAvailableSemaphores[index];
}

vk::Semaphore VulkanInstance::getRenderSemaphore(uint32_t index) const {
  return _renderFinishedSemaphores[index];
}

vk::Fence VulkanInstance::getFence(uint32_t index) const {
  return _inFlightFences[index];
}

void VulkanInstance::waitForFence() const {
  vk::Result result;
  result = _device.waitForFences(1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);
  IF_THROW(
      result != vk::Result::eSuccess,
      failed to waitfor fence...
      );
  result = _device.resetFences(1, &_inFlightFences[_currentFrame]);
  IF_THROW(
      result != vk::Result::eSuccess,
      failed to reset fence...
      );
}

vk::CommandBuffer VulkanInstance::getCommandBufferBegin() const {
  _commandBuffers[_currentFrame].reset();
  vk::CommandBufferBeginInfo beginInfo;
  beginInfo.setFlags(vk::CommandBufferUsageFlags(0));
  beginInfo.setPInheritanceInfo(nullptr);

  _commandBuffers[_currentFrame].begin(beginInfo);

  return _commandBuffers[_currentFrame];
}

void VulkanInstance::getCommandBufferEnd() const {
  _commandBuffers[_currentFrame].end();
}

void VulkanInstance::applyGraphicsQueue() const {
  vk::Result result;
  vk::SubmitInfo submitInfo;

  std::vector<vk::Semaphore> waitSemaphores = { _imageAvailableSemaphores[_currentFrame] };
  std::vector<vk::PipelineStageFlags> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
  std::vector<vk::Semaphore> signalSemaphores = { _renderFinishedSemaphores[_currentFrame] };

  submitInfo.setWaitSemaphores(waitSemaphores);
  submitInfo.setWaitDstStageMask(waitStages);

  submitInfo.setSignalSemaphores(signalSemaphores);
  
  submitInfo.setCommandBufferCount(1);
  submitInfo.setPCommandBuffers(&_commandBuffers[_currentFrame]);

  result = _graphicsQueue.submit(1, &submitInfo, _inFlightFences[_currentFrame]);
  IF_THROW(
      result != vk::Result::eSuccess, 
      failed to submit to GraphicsQueue...
      );
}

void VulkanInstance::applyPresentQueue(uint32_t imageIndex) {
  vk::Result result;
  std::vector<vk::Semaphore> waitSemaphores = { _renderFinishedSemaphores[_currentFrame] };

  vk::PresentInfoKHR presentInfo;
  presentInfo.setWaitSemaphores(waitSemaphores);

  std::vector<vk::SwapchainKHR> swapChains = { _swapChain };

  presentInfo.setSwapchains(swapChains)
             .setPImageIndices(&imageIndex);

  result = _presentQueue.presentKHR(presentInfo);

  if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || _frameBufferResized) {
    recreateSwapChain();
    setFrameBufferResized(false);
  } else if (result != vk::Result::eSuccess) {
      throw std::runtime_error("trouble presenting image...");
  }
}

void VulkanInstance::createInstance() {
  IF_THROW(
      _enableValidationLayers && !validationLayerSupportChecked(validationLayers),
      failed to enable validation layer
      );

  vk::ApplicationInfo appInfo;
  appInfo.setPApplicationName("VulkanInstance Application")
         .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
         .setPEngineName("No Engine")
         .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
         .setApiVersion(VK_API_VERSION_1_0);

  vk::InstanceCreateInfo createInfo;
  createInfo.setPApplicationInfo(&appInfo);
  
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  
  createInfo.setEnabledExtensionCount(glfwExtensionCount);
  createInfo.setPpEnabledExtensionNames(glfwExtensions);

  if (_enableValidationLayers) {
    createInfo.setPEnabledLayerNames(validationLayers);
  } else {
    createInfo.setEnabledLayerCount(0);
  }

  _instance = vk::createInstance(createInfo);
  CHECK_NULL(_instance);
}

void VulkanInstance::createSurface() {
  VkSurfaceKHR surface;
  VkResult result = glfwCreateWindowSurface(static_cast<VkInstance>(_instance), _window, nullptr, &surface);
  IF_THROW(
      result != VK_SUCCESS,
      failed to create surface...
      );
  _surface = vk::SurfaceKHR(surface);
  CHECK_NULL(_surface);
}

void VulkanInstance::pickPhysicalDevice() {
  std::vector<vk::PhysicalDevice> phyDevices = _instance.enumeratePhysicalDevices();
  for (const auto& phyDevice : phyDevices) {
    bool result;
    QueueFamilyIndices* indices;
    std::tie(result, indices) = isDeviceSuitable(phyDevice, _surface, deviceExtensions);
    if (result) {
      _queueIndices = indices;
      _gpu = phyDevice;
      break;
    }
  }
  CHECK_NULL(_gpu);
}

void VulkanInstance::createLogicalDevice() {
  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = { _queueIndices->graphicsFamily.value(), _queueIndices->presentFamily.value() };

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    vk::DeviceQueueCreateInfo queueCreateInfo;
    queueCreateInfo.setQueueFamilyIndex(queueFamily);
    queueCreateInfo.setQueueCount(1);
    queueCreateInfo.setPQueuePriorities(&queuePriority);
    queueCreateInfos.push_back(queueCreateInfo);
  }

  vk::PhysicalDeviceFeatures deviceFeatures;

  vk::DeviceCreateInfo createInfo;
  createInfo.setQueueCreateInfos(queueCreateInfos);
  createInfo.setPEnabledFeatures(&deviceFeatures);
  createInfo.setPEnabledExtensionNames(deviceExtensions);
  createInfo.setEnabledLayerCount(0);

  _device = _gpu.createDevice(createInfo);
  CHECK_NULL(_device);

  _graphicsQueue = _device.getQueue(_queueIndices->graphicsFamily.value(), 0);
  CHECK_NULL(_graphicsQueue);
  _presentQueue = _device.getQueue(_queueIndices->presentFamily.value(), 0);
  CHECK_NULL(_presentQueue);
}

void VulkanInstance::createSwapChain() {
  SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_gpu, _surface);

  vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
  vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
  vk::Extent2D extent = chooseSwapExtent(_window, swapChainSupport.capabilities);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

  if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  vk::SwapchainCreateInfoKHR createInfo;
  createInfo.setSurface(_surface);
  createInfo.setMinImageCount(imageCount);
  createInfo.setImageFormat(surfaceFormat.format);
  createInfo.setImageColorSpace(surfaceFormat.colorSpace);
  createInfo.setImageExtent(extent);
  createInfo.setImageArrayLayers(1);
  createInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
  
  std::vector<uint32_t> queueFamilyIndices = { _queueIndices->graphicsFamily.value(), _queueIndices->presentFamily.value() };

  if (_queueIndices->graphicsFamily != _queueIndices->presentFamily) {
    createInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
    createInfo.setQueueFamilyIndices(queueFamilyIndices);
  } else {
    createInfo.setImageSharingMode(vk::SharingMode::eExclusive);
  }

  createInfo.setPreTransform(swapChainSupport.capabilities.currentTransform);
  createInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
  createInfo.setPresentMode(presentMode);
  createInfo.setClipped(VK_TRUE);

  _swapChain = _device.createSwapchainKHR(createInfo);

  _swapChainImages = _device.getSwapchainImagesKHR(_swapChain);
  _swapChainImageFormat = surfaceFormat.format;
  _swapChainExtent = extent;
}

void VulkanInstance::createImageViews() {
  _swapChainImageViews.resize(_swapChainImages.size());
  for (size_t i = 0; i < _swapChainImages.size(); i++) {
    vk::ImageViewCreateInfo createInfo;
    createInfo.setImage(_swapChainImages[i]);
    createInfo.setViewType(vk::ImageViewType::e2D);
    createInfo.setFormat(_swapChainImageFormat);
    createInfo.setComponents(vk::ComponentMapping(
          vk::ComponentSwizzle::eIdentity,
          vk::ComponentSwizzle::eIdentity,
          vk::ComponentSwizzle::eIdentity,
          vk::ComponentSwizzle::eIdentity)
        );
    createInfo.setSubresourceRange(vk::ImageSubresourceRange( // SUS
          vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        );
    _swapChainImageViews[i] = _device.createImageView(createInfo);
  }
}

void VulkanInstance::createRenderPass() {
  vk::AttachmentDescription colorAttachment;
  colorAttachment.setFormat(_swapChainImageFormat);
  colorAttachment.setSamples(vk::SampleCountFlagBits::e1);
  colorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
  colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
  colorAttachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
  colorAttachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
  colorAttachment.setInitialLayout(vk::ImageLayout::eUndefined);
  colorAttachment.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

  vk::AttachmentReference colorAttachmentRef;
  colorAttachmentRef.setAttachment(0);
  colorAttachmentRef.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

  vk::SubpassDescription subpass;
  subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
  subpass.setColorAttachments(colorAttachmentRef);

  vk::SubpassDependency dependency;
  dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL);
  dependency.setDstSubpass(0);
  dependency.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
  dependency.setSrcAccessMask(vk::AccessFlagBits::eNone);
  dependency.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
  dependency.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

  vk::RenderPassCreateInfo createInfo;
  createInfo.setAttachments(colorAttachment);
  createInfo.setSubpasses(subpass);
  createInfo.setDependencies(dependency);

  _renderPass = _device.createRenderPass(createInfo);
}

void VulkanInstance::createFrameBuffers() {
  _swapChainFramebuffers.resize(_swapChainImageViews.size());
  for (size_t i = 0; i < _swapChainImageViews.size(); i++) {
    std::vector<vk::ImageView> attachments = { _swapChainImageViews[i] };

    vk::FramebufferCreateInfo createInfo;
    createInfo.setRenderPass(_renderPass);
    createInfo.setAttachments(attachments);
    createInfo.setWidth(_swapChainExtent.width);
    createInfo.setHeight(_swapChainExtent.height);
    createInfo.setLayers(1);

    _swapChainFramebuffers[i] = _device.createFramebuffer(createInfo);
  }
}

void VulkanInstance::createCommandPool() {
  vk::CommandPoolCreateInfo createInfo;
  createInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
  createInfo.setQueueFamilyIndex(_queueIndices->graphicsFamily.value());

  _commandPool = _device.createCommandPool(createInfo);
}

void VulkanInstance::allocateCommandBuffers() {
  _commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  
  vk::CommandBufferAllocateInfo allocInfo;
  allocInfo.setCommandPool(_commandPool);
  allocInfo.setLevel(vk::CommandBufferLevel::ePrimary);
  allocInfo.setCommandBufferCount((uint32_t)_commandBuffers.size());

  _commandBuffers = _device.allocateCommandBuffers(allocInfo);
}

void VulkanInstance::createSyncObjects() {
  _imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  _renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  vk::SemaphoreCreateInfo semaphoreInfo;

  vk::FenceCreateInfo fenceInfo;
  fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    _imageAvailableSemaphores[i] = _device.createSemaphore(semaphoreInfo);
    _renderFinishedSemaphores[i] = _device.createSemaphore(semaphoreInfo);
    _inFlightFences[i] = _device.createFence(fenceInfo);
  }
}

void VulkanInstance::cleanupSwapChain() {
  _device.waitIdle();

  for (size_t i = 0; i < _swapChainFramebuffers.size(); i++) {
    _device.destroyFramebuffer(_swapChainFramebuffers[i]);
  }

  for (size_t i = 0; i < _swapChainImageViews.size(); i++) {
    _device.destroyImageView(_swapChainImageViews[i]);
  }

  _device.destroySwapchainKHR(_swapChain);
}

void VulkanInstance::cleanupRenderPass() {
  _device.destroyRenderPass(_renderPass);
}

void VulkanInstance::cleanupSyncObjects() {
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    _device.destroySemaphore(_imageAvailableSemaphores[i]);
    _device.destroySemaphore(_renderFinishedSemaphores[i]);
    _device.destroyFence(_inFlightFences[i]);
  }
}

void VulkanInstance::cleanupCommandPool() {
  _device.destroyCommandPool(_commandPool);
}

void VulkanInstance::cleanupLogicalDevice() {
  _device.destroy();
}

void VulkanInstance::cleanupSurface() {
  _instance.destroySurfaceKHR(_surface);
}

void VulkanInstance::cleanupInstance() {
  _instance.destroy();
}
