#include <optional>
#include <vulkan/vulkan.hpp>

class VulkanInstance;
class RenderAssets;

class VertexRenderer {
public:
  VertexRenderer() = default;
  ~VertexRenderer() = default;
  void init(VulkanInstance* instance, RenderAssets* assets);
  void drawFrame();
private:
  void allocateVertexBuffer();
  void allocateIndexBuffer();
  void allocateUniformBuffer();
  void updateDescriptorSets();
  void updateUniformBuffer(uint32_t currentFrame);
private:
  vk::Device _device;
  vk::PhysicalDevice _gpu;
private:
  std::optional<uint32_t> _vertexIndex;
  std::optional<uint32_t> _indexIndex;
  std::optional<uint32_t> _uniformIndex;
  void* _data;
  std::vector<vk::DescriptorSet> _descriptorSets;
private:
  VulkanInstance* _instance;
  RenderAssets* _assets;
};
