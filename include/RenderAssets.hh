#include <vulkan/vulkan.hpp>

#include <unordered_map>

class VulkanInstance;

class RenderAssets {
public:
  RenderAssets() = default;
  ~RenderAssets() = default;
  void init(VulkanInstance* instance);
  void cleanup();
public:
  vk::Pipeline getGraphicsPipeline() const;
  vk::PipelineLayout getGraphicsPipelineLayout() const;
  vk::DescriptorPool getDescriptorPool() const;
  vk::DescriptorSetLayout getDescriptorSetLayout() const;
  vk::Buffer getBuffer(uint32_t index) const;
public:
  uint32_t createBuffer(
      vk::DeviceSize size, 
      vk::BufferUsageFlags usage, 
      vk::MemoryPropertyFlags memoryProp
      );

  void mapMemory(uint32_t index, vk::DeviceSize size, void** mem);

  void storeBuffer(
      vk::Buffer src, 
      uint32_t dst, 
      vk::DeviceSize size
      );
private:
  uint32_t _index = 0;
private:
  VulkanInstance* _instance = nullptr;
  vk::Device _device = nullptr;
  vk::PhysicalDevice _gpu = nullptr;
  vk::Queue _graphicsQueue = nullptr;
  vk::CommandPool _commandPool = nullptr;
private:
  vk::DescriptorSetLayout _descriptorSetLayout = nullptr;
  vk::PipelineLayout _graphicsPipelineLayout = nullptr;
  vk::Pipeline _graphicsPipeline = nullptr;
  std::unordered_map<uint32_t, vk::Buffer> _buffers;
  std::unordered_map<uint32_t, vk::DeviceMemory> _memories;

  vk::DescriptorPool _descriptorPool = nullptr;
private:
  void createDescriptorSetLayout();
  void createGraphicsPipeline();
  void createDescriptorPool();
private:
  void cleanupDescriptorPool();
  void cleanupDescriptorSetLayout();
  void cleanupGraphicsPipelineLayout();
  void cleanupGraphicsPipeline();
  void cleanupBufferMemory();
};
