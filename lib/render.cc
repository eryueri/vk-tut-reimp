#include "render.hh"

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
  const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
  };

  const std::vector<const char*> deviceExtensions = {
    "VK_KHR_swapchain"
  };
};

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

void BaseRender::connectWindow(GLFWwindow* w) {
  window = w;
}

void BaseRender::init() {
  createInstance();
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();
}

void BaseRender::drawFrame() {

}

void BaseRender::cleanup() {
  device.destroy();
  instance.destroySurfaceKHR();
  instance.destroy();
}

void BaseRender::createInstance() {
  IF_THROW(
      enableValidationLayers && !HelperFunc::validationLayerSupportChecked(globalTables::validationLayers),
      failed to enable validation layer
      )

  vk::ApplicationInfo appInfo;
  appInfo.setPApplicationName("BaseRender Application");
  appInfo.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0));
  appInfo.setPEngineName("No Engine");
  appInfo.setEngineVersion(VK_MAKE_VERSION(1, 0, 0));
  appInfo.setApiVersion(VK_API_VERSION_1_0);

  vk::InstanceCreateInfo createInfo;
  createInfo.setPApplicationInfo(&appInfo);
  createInfo.setEnabledExtensionCount(0);

  if (enableValidationLayers) {
    createInfo.setEnabledLayerCount(static_cast<uint32_t>(globalTables::validationLayers.size()));
    createInfo.setPpEnabledLayerNames(globalTables::validationLayers.data());
  } else {
    createInfo.setEnabledLayerCount(0);
  }

  instance = vk::createInstance(createInfo);
}

void BaseRender::createSurface() {
  VkSurfaceKHR _surface;
  glfwCreateWindowSurface(static_cast<VkInstance>(instance), window, nullptr, &_surface);
  surface = vk::SurfaceKHR(_surface);
}

void BaseRender::pickPhysicalDevice() {
  std::vector<vk::PhysicalDevice> phyDevices = instance.enumeratePhysicalDevices();
  for (const auto& phyDevice : phyDevices) {
    if (HelperFunc::isDeviceSuitable(phyDevice, surface)) {
      physicalDevice = phyDevice;
      break;
    }
  }
}

void BaseRender::createLogicalDevice() {
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

void BaseRender::createSwapChain() {
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
  createInfo.setClipped(vk::Bool32(true));

  swapChain = device.createSwapchainKHR(createInfo);

  swapChainImages = device.getSwapchainImagesKHR(swapChain);
  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent = extent;
}

void BaseRender::createImageViews() {
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
    createInfo.setSubresourceRange(vk::ImageSubresourceRange(
          vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        );
    swapChainImageViews[i] = device.createImageView(createInfo);
  }
}

void BaseRender::createRenderPass() {
  vk::AttachmentDescription colorAttachment;
  colorAttachment.setFormat(swapChainImageFormat);
  colorAttachment.setSamples(vk::SampleCountFlagBits::e1);
  colorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
  colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
  colorAttachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
  colorAttachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
  colorAttachment.setInitialLayout(vk::ImageLayout::eUndefined);
  colorAttachment.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
}

void BaseRender::createGraphicsPipeline() {

}

void BaseRender::createFrameBuffers() {

}

void BaseRender::createCommandPool() {

}

void BaseRender::createCommandBuffer() {

}

void BaseRender::createSyncObjects() {

}
