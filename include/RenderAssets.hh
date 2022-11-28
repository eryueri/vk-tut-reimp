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
  const vk::DescriptorSet* getDescriptorSet() const;
  vk::Buffer getBuffer(uint32_t index) const;
  vk::Image getImage(uint32_t index) const;
public:
  uint32_t createImage(
      uint32_t width, 
      uint32_t height, 
      vk::Format format, 
      vk::ImageTiling tiling, 
      vk::ImageUsageFlags usage, 
      vk::MemoryPropertyFlags memoryProp
      );

  uint32_t createBuffer(
      vk::DeviceSize size, 
      vk::BufferUsageFlags usage, 
      vk::MemoryPropertyFlags memoryProp
      );

  void mapMemory(uint32_t index, vk::DeviceSize size, void** mem);

  void updateDescriptorSets(uint32_t index);

  void storeBuffer(
      vk::Buffer src, 
      uint32_t dst, 
      vk::DeviceSize size
      );

  void storeBufferToImage(
      vk::Buffer src,
      uint32_t dst, 
      const uint32_t& width, 
      const uint32_t& height
      );
private:
  uint32_t _bufferIndex = 0;
  uint32_t _imageIndex = 0;
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
  std::unordered_map<uint32_t, vk::Image> _images;
  std::unordered_map<uint32_t, vk::DeviceMemory> _imageMemories;

  vk::DescriptorPool _descriptorPool = nullptr;
  std::vector<vk::DescriptorSet> _descriptorSets;
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
  void cleanupImageMemory();
};
