#include <optional>
#include <vulkan/vulkan.hpp>

class VulkanInstance;
class RenderAssets;

class Renderer {
public:
  Renderer() = default;
  ~Renderer() = default;
  void init(VulkanInstance* instance, RenderAssets* assets);
  void drawFrame();
private:
  void createTextureImage();
  void allocateVertexBuffer();
  void allocateIndexBuffer();
  void allocateUniformBuffer();
  void updateUniformBuffer(uint32_t currentFrame);
  void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
private:
  vk::Device _device;
  vk::PhysicalDevice _gpu;
private:
  std::optional<uint32_t> _vertexIndex;
  std::optional<uint32_t> _indexIndex;
  std::optional<uint32_t> _uniformIndex;
  std::optional<uint32_t> _imageIndex;
  void* _data;
private:
  VulkanInstance* _instance;
  RenderAssets* _assets;
};
