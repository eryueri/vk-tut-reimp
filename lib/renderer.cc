#include "renderer.hh"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <set>
#include <limits>

#include "utils.hh"

#ifdef DEBUG
  const bool enableValidationLayers = true;
#else
  const bool enableValidationLayers = false;
#endif

#define IF_THROW(expr, message) \
  if ((expr)) { \
    throw std::runtime_error(#message); \
  }

namespace globalTables {
  static const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

  const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
  };

  const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };

  const std::vector<Vertex> vertices = {
    { {  0.0f, -0.5f }, { 0.6f, 0.3f, 0.8f } },
    { {  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f } },
    { { -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f } },
  };
};

vk::VertexInputBindingDescription Vertex::getBindingDescription() {
  vk::VertexInputBindingDescription description{};
  description.setBinding(0)
             .setStride(sizeof(Vertex))
             .setInputRate(vk::VertexInputRate::eVertex);

  return description;
}

std::array<vk::VertexInputAttributeDescription, 2> Vertex::getAttributeDescriptions() {
  std::array<vk::VertexInputAttributeDescription, 2> descriptions;
  descriptions[0].setBinding(0)
                 .setLocation(0)
                 .setFormat(vk::Format::eR32G32Sfloat)
                 .setOffset(offsetof(Vertex, pos));
  descriptions[1].setBinding(0)
                 .setLocation(1)
                 .setFormat(vk::Format::eR32G32B32Sfloat)
                 .setOffset(offsetof(Vertex, color));

  return descriptions;
}

bool QueueFamilyIndices::isComplete() {
  return graphicsFamily.has_value() && presentFamily.has_value();
}

bool HelperFunc::validationLayerSupportChecked(const std::vector<const char*> validationLayers) {
  std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

  std::set<std::string> requiredValidationLayers(validationLayers.begin(), validationLayers.end());

  for (const auto& layerProperties : availableLayers) {
    requiredValidationLayers.erase(layerProperties.layerName);
  }

  return requiredValidationLayers.empty();
}

bool HelperFunc::deviceExtensionSupportChecked(vk::PhysicalDevice device, const std::vector<const char*> deviceExtensions) {
  std::vector<vk::ExtensionProperties> supportedExtensions = device.enumerateDeviceExtensionProperties();

  std::set<std::string> requiredDeviceExtensions(deviceExtensions.begin(), deviceExtensions.end());

  for (const auto& extensionProperties : supportedExtensions) {
    requiredDeviceExtensions.erase(extensionProperties.extensionName);
  }

  return requiredDeviceExtensions.empty();
}

bool HelperFunc::isDeviceSuitable(vk::PhysicalDevice phyDevice, vk::SurfaceKHR surface) {
  QueueFamilyIndices indices = findQueueFamilies(phyDevice, surface);

  bool extensionSupported = deviceExtensionSupportChecked(phyDevice, globalTables::deviceExtensions);

  bool swapChainAdequate = false;

  if (extensionSupported) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(phyDevice, surface);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  return indices.isComplete() && extensionSupported && swapChainAdequate;
}

uint32_t HelperFunc::findMemType(uint32_t typeFilter, vk::MemoryPropertyFlags prop, const vk::PhysicalDevice& device) {
  vk::PhysicalDeviceMemoryProperties memProp = device.getMemoryProperties();
  for (uint32_t i = 0; i < memProp.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProp.memoryTypes[i].propertyFlags & prop) == prop) {
        return i;
    }
  }
  throw std::runtime_error("can not find proper mem type yo!");
}

QueueFamilyIndices HelperFunc::findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
  QueueFamilyIndices indices;

  std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

  int i = 0;
  for (const auto& queueFamily : queueFamilies) {
    if (device.getSurfaceSupportKHR(i, surface)) {
      indices.presentFamily = i;
    }
    if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
      indices.graphicsFamily = i;
    }
    if (indices.isComplete()) break;
    i++;
  }

  return indices;
}

SwapChainSupportDetails HelperFunc::querySwapChainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
  SwapChainSupportDetails details;
  details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
  details.formats = device.getSurfaceFormatsKHR(surface);
  details.presentModes = device.getSurfacePresentModesKHR(surface);
  return details;
}

vk::ShaderModule HelperFunc::createShaderModule(vk::Device device, const std::vector<char>& code) {
  vk::ShaderModule shaderModule;
  vk::ShaderModuleCreateInfo createInfo;
  createInfo.setCodeSize(code.size());
  createInfo.setPCode(reinterpret_cast<const uint32_t*>(code.data()));

  shaderModule = device.createShaderModule(createInfo);
  return shaderModule;
}

vk::SurfaceFormatKHR HelperFunc::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
  for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return availableFormat;
    }
  }
  return availableFormats[0];
}

vk::PresentModeKHR HelperFunc::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
  for (const auto& availablePresentMode : availablePresentModes) {
    if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
      return availablePresentMode;
    }
  }
  return vk::PresentModeKHR::eFifo;
}

vk::Extent2D HelperFunc::chooseSwapExtent(GLFWwindow* window, const vk::SurfaceCapabilitiesKHR& capabilities) {
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    vk::Extent2D actualExtent = {
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height)
    };
    
    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

BaseRenderer::BaseRenderer() {}

BaseRenderer::~BaseRenderer() {}

void BaseRenderer::connectWindow(GLFWwindow* w) {
  window = w;
}

void BaseRenderer::init() {
  createInstance();
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();
  createSwapChain();
  createImageViews();
  createRenderPass();
  createGraphicsPipeline();
  createFrameBuffers();
  createCommandPool();
  allocateCommandBuffers();
  allocateVertexBuffer();
  createSyncObjects();
}

void BaseRenderer::drawFrame() {
  static uint32_t currentFrame = 0;
  vk::resultCheck(
      device.waitForFences(1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX), 
      "error when waiting for fence..."
      );
  
  uint32_t imageIndex;
  vk::Result acquireResult = device.acquireNextImageKHR(swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], nullptr, &imageIndex);

  if (acquireResult == vk::Result::eErrorOutOfDateKHR) {
    recreateSwapChain();
    return;
  } else if (acquireResult != vk::Result::eSuccess && acquireResult != vk::Result::eSuboptimalKHR) {
    throw std::runtime_error("trouble acquiring next image...");
  }

  vk::resultCheck(
      device.resetFences(1, &inFlightFences[currentFrame]),
      "error when reseting fence..."
      );

  commandBuffers[currentFrame].reset();
  recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

  vk::SubmitInfo submitInfo;

  std::vector<vk::Semaphore> waitSemaphores = { imageAvailableSemaphores[currentFrame] };
  std::vector<vk::PipelineStageFlags> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
  std::vector<vk::Semaphore> signalSemaphores = { renderFinishedSemaphores[currentFrame] };

  submitInfo.setWaitSemaphores(waitSemaphores);
  submitInfo.setWaitDstStageMask(waitStages);

  submitInfo.setSignalSemaphores(signalSemaphores);
  
  submitInfo.setCommandBufferCount(1);
  submitInfo.setPCommandBuffers(&commandBuffers[currentFrame]);

  vk::resultCheck(
      graphicQueue.submit(1, &submitInfo, inFlightFences[currentFrame]),
      "trouble submitting commandbuffer..."
      );

  vk::PresentInfoKHR presentInfo;
  presentInfo.setWaitSemaphores(signalSemaphores);

  std::vector<vk::SwapchainKHR> swapChains = { swapChain };

  presentInfo.setSwapchains(swapChains);

  presentInfo.setPImageIndices(&imageIndex);

  vk::Result presentResult = presentQueue.presentKHR(presentInfo);

  if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR || framebufferResized) {
    recreateSwapChain();
    setFramebufferResized(false);
  } else if (presentResult != vk::Result::eSuccess) {
      throw std::runtime_error("trouble presenting image...");
  }

  currentFrame = (currentFrame + 1) % globalTables::MAX_FRAMES_IN_FLIGHT;
}

void BaseRenderer::cleanup() {
  cleanupSwapChain();

  device.destroyBuffer(vertexBuffer);
  device.freeMemory(vertexBufferMem);

  device.destroyPipeline(graphicsPipeline);
  device.destroyPipelineLayout(pipelineLayout);
  device.destroyRenderPass(renderPass);

  for (size_t i = 0; i < globalTables::MAX_FRAMES_IN_FLIGHT; i++) {
    device.destroySemaphore(imageAvailableSemaphores[i]);
    device.destroySemaphore(renderFinishedSemaphores[i]);
    device.destroyFence(inFlightFences[i]);
  }

  device.destroyCommandPool(commandPool);
  device.destroy();

  instance.destroySurfaceKHR(surface);
  instance.destroy();
}

void BaseRenderer::setFramebufferResized(bool v) {
  framebufferResized = v;
}

void BaseRenderer::createInstance() {
  IF_THROW(
      enableValidationLayers && !HelperFunc::validationLayerSupportChecked(globalTables::validationLayers),
      failed to enable validation layer
      )

  vk::ApplicationInfo appInfo;
  appInfo.setPApplicationName("BaseRenderer Application")
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

  if (enableValidationLayers) {
    createInfo.setPEnabledLayerNames(globalTables::validationLayers);
  } else {
    createInfo.setEnabledLayerCount(0);
  }

  instance = vk::createInstance(createInfo);
}

void BaseRenderer::createSurface() {
  VkSurfaceKHR _surface;
  VkResult result = glfwCreateWindowSurface(static_cast<VkInstance>(instance), window, nullptr, &_surface);
  IF_THROW(
      result != VK_SUCCESS,
      failed to create surface...
      )
  surface = vk::SurfaceKHR(_surface);
}

void BaseRenderer::pickPhysicalDevice() {
  std::vector<vk::PhysicalDevice> phyDevices = instance.enumeratePhysicalDevices();
  for (const auto& phyDevice : phyDevices) {
    if (HelperFunc::isDeviceSuitable(phyDevice, surface)) {
      physicalDevice = phyDevice;
      break;
    }
  }
}

void BaseRenderer::createLogicalDevice() {
  QueueFamilyIndices indices = HelperFunc::findQueueFamilies(physicalDevice, surface);

  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

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
  createInfo.setPEnabledExtensionNames(globalTables::deviceExtensions);
  createInfo.setEnabledLayerCount(0);

  device = physicalDevice.createDevice(createInfo);

  device.getQueue(indices.graphicsFamily.value(), 0, &graphicQueue);
  device.getQueue(indices.presentFamily.value(), 0, &presentQueue);
}

void BaseRenderer::createSwapChain() {
  SwapChainSupportDetails swapChainSupport = HelperFunc::querySwapChainSupport(physicalDevice, surface);

  vk::SurfaceFormatKHR surfaceFormat = HelperFunc::chooseSwapSurfaceFormat(swapChainSupport.formats);
  vk::PresentModeKHR presentMode = HelperFunc::chooseSwapPresentMode(swapChainSupport.presentModes);
  vk::Extent2D extent = HelperFunc::chooseSwapExtent(window, swapChainSupport.capabilities);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

  if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  vk::SwapchainCreateInfoKHR createInfo;
  createInfo.setSurface(surface);
  createInfo.setMinImageCount(imageCount);
  createInfo.setImageFormat(surfaceFormat.format);
  createInfo.setImageColorSpace(surfaceFormat.colorSpace);
  createInfo.setImageExtent(extent);
  createInfo.setImageArrayLayers(1);
  createInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
  
  QueueFamilyIndices indices = HelperFunc::findQueueFamilies(physicalDevice, surface);
  std::vector<uint32_t> queueFamilyIndices = { indices.graphicsFamily.value(), indices.presentFamily.value() };

  if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
    createInfo.setQueueFamilyIndices(queueFamilyIndices);
  } else {
    createInfo.setImageSharingMode(vk::SharingMode::eExclusive);
  }

  createInfo.setPreTransform(swapChainSupport.capabilities.currentTransform);
  createInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
  createInfo.setPresentMode(presentMode);
  createInfo.setClipped(VK_TRUE);

  swapChain = device.createSwapchainKHR(createInfo);

  swapChainImages = device.getSwapchainImagesKHR(swapChain);
  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent = extent;
}

void BaseRenderer::createImageViews() {
  swapChainImageViews.resize(swapChainImages.size());
  for (size_t i = 0; i < swapChainImages.size(); i++) {
    vk::ImageViewCreateInfo createInfo;
    createInfo.setImage(swapChainImages[i]);
    createInfo.setViewType(vk::ImageViewType::e2D);
    createInfo.setFormat(swapChainImageFormat);
    createInfo.setComponents(vk::ComponentMapping(
          vk::ComponentSwizzle::eIdentity,
          vk::ComponentSwizzle::eIdentity,
          vk::ComponentSwizzle::eIdentity,
          vk::ComponentSwizzle::eIdentity)
        );
    createInfo.setSubresourceRange(vk::ImageSubresourceRange( // SUS
          vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        );
    swapChainImageViews[i] = device.createImageView(createInfo);
  }
}

void BaseRenderer::createRenderPass() {
  vk::AttachmentDescription colorAttachment;
  colorAttachment.setFormat(swapChainImageFormat);
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

  renderPass = device.createRenderPass(createInfo);
}

void BaseRenderer::createGraphicsPipeline() {
  auto vertShaderCode = myUtils::readBinaryFile("../glslShaders/build/vert.spv");
  auto fragShaderCode = myUtils::readBinaryFile("../glslShaders/build/frag.spv");

  vk::ShaderModule vertShaderModule = HelperFunc::createShaderModule(device, vertShaderCode);
  vk::ShaderModule fragShaderModule = HelperFunc::createShaderModule(device, fragShaderCode);

  vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
  vertShaderStageInfo.setStage(vk::ShaderStageFlagBits::eVertex);
  vertShaderStageInfo.setModule(vertShaderModule);
  vertShaderStageInfo.setPName("main");

  vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
  fragShaderStageInfo.setStage(vk::ShaderStageFlagBits::eFragment);
  fragShaderStageInfo.setModule(fragShaderModule);
  fragShaderStageInfo.setPName("main");

  vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

  vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
  const auto bindingDesc = Vertex::getBindingDescription();
  const auto attribDesc = Vertex::getAttributeDescriptions();
  vertexInputInfo.setVertexBindingDescriptions(bindingDesc)
                 .setVertexAttributeDescriptions(attribDesc);

  vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
  inputAssembly.setTopology(vk::PrimitiveTopology::eTriangleList);
  inputAssembly.setPrimitiveRestartEnable(VK_FALSE);

  vk::Viewport viewport;
  viewport.setX(0.0f);
  viewport.setY(0.0f);
  viewport.setWidth(static_cast<float>(swapChainExtent.width));
  viewport.setHeight(static_cast<float>(swapChainExtent.height));
  viewport.setMinDepth(0.0f);
  viewport.setMaxDepth(1.0f);

  vk::Rect2D scissor;
  scissor.setOffset(vk::Offset2D(0, 0));
  scissor.setExtent(swapChainExtent);

  std::vector<vk::DynamicState> dynamicStates = {
    vk::DynamicState::eViewport,
    vk::DynamicState::eScissor
  };

  vk::PipelineDynamicStateCreateInfo dynamicState;
  dynamicState.setDynamicStates(dynamicStates);

  vk::PipelineViewportStateCreateInfo viewportState;
  viewportState.setViewports(viewport);
  viewportState.setScissors(scissor);

  vk::PipelineRasterizationStateCreateInfo rasterizer;
  rasterizer.setDepthClampEnable(VK_FALSE);
  rasterizer.setRasterizerDiscardEnable(VK_FALSE);
  rasterizer.setPolygonMode(vk::PolygonMode::eFill);
  rasterizer.setLineWidth(1.0f);
  rasterizer.setCullMode(vk::CullModeFlagBits::eBack);
  rasterizer.setFrontFace(vk::FrontFace::eClockwise);
  rasterizer.setDepthBiasEnable(VK_FALSE);

  vk::PipelineMultisampleStateCreateInfo multisampling;
  multisampling.setSampleShadingEnable(VK_FALSE);
  multisampling.setRasterizationSamples(vk::SampleCountFlagBits::e1);

  vk::PipelineColorBlendAttachmentState colorBlendAttachment;
  colorBlendAttachment.setColorWriteMask( // SUS
      vk::ColorComponentFlagBits::eR | 
      vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB |
      vk::ColorComponentFlagBits::eA
      );
  colorBlendAttachment.setBlendEnable(VK_FALSE);

  vk::PipelineColorBlendStateCreateInfo colorBlending;
  colorBlending.setLogicOpEnable(VK_FALSE);
  colorBlending.setAttachments(colorBlendAttachment);

  vk::PipelineLayoutCreateInfo pipelineLayoutInfo;

  pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

  vk::GraphicsPipelineCreateInfo pipelineInfo;
  pipelineInfo.setStages(shaderStages);
  pipelineInfo.setPVertexInputState(&vertexInputInfo);
  pipelineInfo.setPInputAssemblyState(&inputAssembly);
  pipelineInfo.setPViewportState(&viewportState);
  pipelineInfo.setPRasterizationState(&rasterizer);
  pipelineInfo.setPMultisampleState(&multisampling);
  pipelineInfo.setPColorBlendState(&colorBlending);
  pipelineInfo.setPDynamicState(&dynamicState);
  pipelineInfo.setLayout(pipelineLayout);
  pipelineInfo.setRenderPass(renderPass);
  pipelineInfo.setSubpass(0);

  vk::Result result;

  std::tie(result, graphicsPipeline) = device.createGraphicsPipeline(nullptr, pipelineInfo);

  IF_THROW(
      result != vk::Result::eSuccess,
      failed to create graphicsPipeline...
      )

  device.destroyShaderModule(fragShaderModule);
  device.destroyShaderModule(vertShaderModule);
}

void BaseRenderer::createFrameBuffers() {
  swapChainFramebuffers.resize(swapChainImageViews.size());
  for (size_t i = 0; i < swapChainImageViews.size(); i++) {
    std::vector<vk::ImageView> attachments = { swapChainImageViews[i] };

    vk::FramebufferCreateInfo createInfo;
    createInfo.setRenderPass(renderPass);
    createInfo.setAttachments(attachments);
    createInfo.setWidth(swapChainExtent.width);
    createInfo.setHeight(swapChainExtent.height);
    createInfo.setLayers(1);

    swapChainFramebuffers[i] = device.createFramebuffer(createInfo);
  }
}

void BaseRenderer::createCommandPool() {
  QueueFamilyIndices indices = HelperFunc::findQueueFamilies(physicalDevice, surface);

  vk::CommandPoolCreateInfo createInfo;
  createInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
  createInfo.setQueueFamilyIndex(indices.graphicsFamily.value());

  commandPool = device.createCommandPool(createInfo);
}

void BaseRenderer::allocateVertexBuffer() {
  vk::BufferCreateInfo createInfo;
  createInfo.setSize(sizeof(globalTables::vertices))
            .setUsage(vk::BufferUsageFlagBits::eVertexBuffer)
            .setSharingMode(vk::SharingMode::eExclusive);

  vertexBuffer = device.createBuffer(createInfo);

  vk::MemoryRequirements memRequirement = device.getBufferMemoryRequirements(vertexBuffer);

  vk::MemoryAllocateInfo alloInfo;
  alloInfo.setAllocationSize(memRequirement.size)
          .setMemoryTypeIndex(HelperFunc::findMemType(memRequirement.memoryTypeBits, 
                              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, 
                              physicalDevice));
  vertexBufferMem = device.allocateMemory(alloInfo);

  device.bindBufferMemory(vertexBuffer, vertexBufferMem, 0);

  void* data;
  auto result = device.mapMemory(vertexBufferMem, 0, createInfo.size, vk::MemoryMapFlags(0), &data);
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error("i do not quiet know why i have to call constractor of vk::MemoryMapFlags instead of just use the number 0, and now it is throwing an error! HOW STUPID!");
  }
    memcpy(data, globalTables::vertices.data(), (size_t)createInfo.size);
  device.unmapMemory(vertexBufferMem);
}

void BaseRenderer::allocateCommandBuffers() {
  commandBuffers.resize(globalTables::MAX_FRAMES_IN_FLIGHT);
  
  vk::CommandBufferAllocateInfo allocInfo;
  allocInfo.setCommandPool(commandPool);
  allocInfo.setLevel(vk::CommandBufferLevel::ePrimary);
  allocInfo.setCommandBufferCount((uint32_t)commandBuffers.size());

  commandBuffers = device.allocateCommandBuffers(allocInfo);
}

void BaseRenderer::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) {
  vk::CommandBufferBeginInfo beginInfo;
  beginInfo.setFlags(vk::CommandBufferUsageFlags(0));
  beginInfo.setPInheritanceInfo(nullptr);

  commandBuffer.begin(beginInfo);

  vk::RenderPassBeginInfo renderPassInfo;
  renderPassInfo.setRenderPass(renderPass);
  renderPassInfo.setFramebuffer(swapChainFramebuffers[imageIndex]);
  renderPassInfo.setRenderArea( // SUS
      vk::Rect2D(
          {0, 0},
          swapChainExtent
        )
      );
  vk::ClearValue clearColor(vk::ClearColorValue().setFloat32({0.0f, 0.0f, 0.0f, 0.5f}));
  renderPassInfo.setClearValues(clearColor);

  commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

  commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

  std::vector<vk::Buffer> vertexBuffers = { vertexBuffer };
  std::vector<vk::DeviceSize> offsets = { 0 };

  commandBuffer.bindVertexBuffers(0, 1, vertexBuffers.data(), offsets.data());

  vk::Viewport viewport;
  viewport.setX(0.0f);
  viewport.setY(0.0f);
  viewport.setWidth(static_cast<float>(swapChainExtent.width));
  viewport.setHeight(static_cast<float>(swapChainExtent.height));
  viewport.setMinDepth(0.0f);
  viewport.setMaxDepth(1.0f);
  
  commandBuffer.setViewport(0, 1, &viewport);

  vk::Rect2D scissor;
  scissor.setOffset(vk::Offset2D(0, 0));
  scissor.setExtent(swapChainExtent);
  
  commandBuffer.setScissor(0, 1, &scissor);

  commandBuffer.draw(static_cast<uint32_t>(globalTables::vertices.size()), 1, 0, 0);

  commandBuffer.endRenderPass();

  commandBuffer.end();
}

void BaseRenderer::createSyncObjects() {
  imageAvailableSemaphores.resize(globalTables::MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores.resize(globalTables::MAX_FRAMES_IN_FLIGHT);
  inFlightFences.resize(globalTables::MAX_FRAMES_IN_FLIGHT);

  vk::SemaphoreCreateInfo semaphoreInfo;

  vk::FenceCreateInfo fenceInfo;
  fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);

  for (size_t i = 0; i < globalTables::MAX_FRAMES_IN_FLIGHT; i++) {
    imageAvailableSemaphores[i] = device.createSemaphore(semaphoreInfo);
    renderFinishedSemaphores[i] = device.createSemaphore(semaphoreInfo);
    inFlightFences[i] = device.createFence(fenceInfo);
  }
}

void BaseRenderer::cleanupSwapChain() {
  device.waitIdle();

  for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
    device.destroyFramebuffer(swapChainFramebuffers[i]);
  }

  for (size_t i = 0; i < swapChainImageViews.size(); i++) {
    device.destroyImageView(swapChainImageViews[i]);
  }

  device.destroySwapchainKHR(swapChain);
}

void BaseRenderer::recreateSwapChain() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  while(width == 0 || height == 0) {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();  
  }

  cleanupSwapChain();

  createSwapChain();
  createImageViews();
  createFrameBuffers();
}
