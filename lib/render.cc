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
