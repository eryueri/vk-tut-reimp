#include <vulkan/vulkan.hpp>

struct QueueFamilyIndices;
struct SwapChainSupportDetails;

class GLFWwindow;

namespace myUtils {

  std::vector<char> readBinaryFile(const std::string& filename);

  bool validationLayerSupportChecked(const std::vector<const char*> validationLayers);
  bool deviceExtensionSupportChecked(vk::PhysicalDevice device, const std::vector<const char*> deviceExtension);

  QueueFamilyIndices* findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface);
  SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface);

  std::tuple<bool, QueueFamilyIndices*> isDeviceSuitable(vk::PhysicalDevice phyDevice, vk::SurfaceKHR surface, const std::vector<const char*> deviceExtensions);

  vk::ShaderModule createShaderModule(vk::Device device, const std::vector<char>& code);

  vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
  vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
  vk::Extent2D chooseSwapExtent(GLFWwindow* window, const vk::SurfaceCapabilitiesKHR& capabilities);

  uint32_t findMemType(uint32_t typeFilter, vk::MemoryPropertyFlags prop, const vk::PhysicalDevice& device);

  std::tuple<vk::Buffer, vk::DeviceMemory> createStagingBuffer(
      vk::DeviceSize bufferSize, 
      vk::Device device, 
      vk::PhysicalDevice physicalDevice);

  vk::CommandBuffer beginSingleTimeCommands(vk::Device device, vk::CommandPool commandPool);

  void submitSingleTimeCommands(vk::CommandBuffer commandBuffer, vk::Queue queue, vk::Device device, vk::CommandPool commandPool);

  vk::ImageView createImageView(vk::Image image, vk::Format format, vk::Device device);

};
