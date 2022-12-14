#include "VkUtils.hh"

#include <set>
#include <tuple>
#include <fstream>

#include <GLFW/glfw3.h>

#include "Structs.hh"

namespace myUtils {

  std::vector<char> readBinaryFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
      throw std::runtime_error("failed to open: " + filename);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
  }

  bool validationLayerSupportChecked(const std::vector<const char*> validationLayers) {
    std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    std::set<std::string> requiredValidationLayers(validationLayers.begin(), validationLayers.end());

    for (const auto& layerProperties : availableLayers) {
      requiredValidationLayers.erase(layerProperties.layerName);
    }

    return requiredValidationLayers.empty();
  }

  bool deviceExtensionSupportChecked(vk::PhysicalDevice device, const std::vector<const char*> deviceExtensions) {
    std::vector<vk::ExtensionProperties> supportedExtensions = device.enumerateDeviceExtensionProperties();

    std::set<std::string> requiredDeviceExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extensionProperties : supportedExtensions) {
      requiredDeviceExtensions.erase(extensionProperties.extensionName);
    }

    return requiredDeviceExtensions.empty();
  }

  QueueFamilyIndices* findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    QueueFamilyIndices* indices = new QueueFamilyIndices;

    std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
      if (device.getSurfaceSupportKHR(i, surface)) {
        indices->presentFamily = i;
      }
      if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
        indices->graphicsFamily = i;
      }
      if (indices->isComplete()) break;
      i++;
    }

    return indices;
  }

  SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    SwapChainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
    details.formats = device.getSurfaceFormatsKHR(surface);
    details.presentModes = device.getSurfacePresentModesKHR(surface);
    return details;
  }

  std::tuple<bool, QueueFamilyIndices*> isDeviceSuitable(vk::PhysicalDevice phyDevice, vk::SurfaceKHR surface, const std::vector<const char*> deviceExtensions) {
    QueueFamilyIndices* indices = findQueueFamilies(phyDevice, surface);

    bool extensionSupported = deviceExtensionSupportChecked(phyDevice, deviceExtensions);

    bool swapChainAdequate = false;

    if (extensionSupported) {
      SwapChainSupportDetails swapChainSupport = querySwapChainSupport(phyDevice, surface);
      swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    // i would not check SamplerAnisotropy feature here cause i'm too lazy
    if (indices->isComplete() 
        && extensionSupported 
        && swapChainAdequate) 
      return std::tuple<bool, QueueFamilyIndices*>(true, indices);
    delete indices;
    return std::tuple<bool, QueueFamilyIndices*>(false, nullptr);
  }

  vk::ShaderModule createShaderModule(vk::Device device, const std::vector<char>& code) {
    vk::ShaderModule shaderModule;
    vk::ShaderModuleCreateInfo createInfo;
    createInfo.setCodeSize(code.size());
    createInfo.setPCode(reinterpret_cast<const uint32_t*>(code.data()));

    shaderModule = device.createShaderModule(createInfo);
    return shaderModule;
  }

  vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
      if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
        return availableFormat;
      }
    }
    return availableFormats[0];
  }

  vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
      if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
        return availablePresentMode;
      }
    }
    return vk::PresentModeKHR::eFifo;
  }

  vk::Extent2D chooseSwapExtent(GLFWwindow* window, const vk::SurfaceCapabilitiesKHR& capabilities) {
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

  uint32_t findMemType(uint32_t typeFilter, vk::MemoryPropertyFlags prop, const vk::PhysicalDevice& device) {
    vk::PhysicalDeviceMemoryProperties memProp = device.getMemoryProperties();
    for (uint32_t i = 0; i < memProp.memoryTypeCount; i++) {
      if ((typeFilter & (1 << i)) && (memProp.memoryTypes[i].propertyFlags & prop) == prop) {
          return i;
      }
    }
    throw std::runtime_error("can not find proper mem type yo!");
  }


  std::tuple<vk::Buffer, vk::DeviceMemory> createStagingBuffer(
      vk::DeviceSize bufferSize, 
      vk::Device device, 
      vk::PhysicalDevice physicalDevice) {
    vk::Buffer buffer;
    vk::DeviceMemory memory;

    vk::BufferCreateInfo bufferInfo;
    bufferInfo.setSize(bufferSize)
              .setUsage(vk::BufferUsageFlagBits::eTransferSrc)
              .setSharingMode(vk::SharingMode::eExclusive);

    buffer = device.createBuffer(bufferInfo);

    vk::MemoryRequirements memRequirement = device.getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo memoryInfo;
    memoryInfo.setAllocationSize(memRequirement.size)
              .setMemoryTypeIndex(findMemType(
                    memRequirement.memoryTypeBits, 
                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, 
                    physicalDevice));

    memory = device.allocateMemory(memoryInfo);

    device.bindBufferMemory(buffer, memory, 0);

    return std::tuple(buffer, memory);
  }

  vk::CommandBuffer beginSingleTimeCommands(vk::Device device, vk::CommandPool commandPool) {
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.setLevel(vk::CommandBufferLevel::ePrimary)
             .setCommandPool(commandPool)
             .setCommandBufferCount(1);

    vk::CommandBuffer cmdBuffer = device.allocateCommandBuffers(allocInfo)[0];

    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    cmdBuffer.begin(beginInfo);

    return cmdBuffer;
  }

  void submitSingleTimeCommands(vk::CommandBuffer commandBuffer, vk::Queue queue, vk::Device device, vk::CommandPool commandPool) {
    commandBuffer.end();

    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(commandBuffer);

    queue.submit(submitInfo);
    queue.waitIdle();

    device.freeCommandBuffers(commandPool, 1, &commandBuffer);
  }

  vk::ImageView createImageView(vk::Image image, vk::Format format, vk::Device device) {
    vk::ImageViewCreateInfo createInfo;
    createInfo.setImage(image)
              .setViewType(vk::ImageViewType::e2D)
              .setFormat(format)
              .setSubresourceRange(vk::ImageSubresourceRange(
                    vk::ImageAspectFlagBits::eColor, 
                    0, 1, 0, 1
                    ));

    vk::ImageView imageView = device.createImageView(createInfo);

    return imageView;
  }

};
