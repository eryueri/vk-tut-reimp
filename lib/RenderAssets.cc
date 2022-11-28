#include "RenderAssets.hh"

#include <vulkan/vulkan.hpp>

#include "VulkanInstance.hh"
#include "VkUtils.hh"
#include "Structs.hh"
#include "Macros.hh"

#define IF_THROW(expr, message) \
  if ((expr)) { \
    throw std::runtime_error(#message); \
  }

void RenderAssets::init(VulkanInstance* instance) {
  _instance = instance;
  _device = _instance->getLogicalDevice();
  _gpu = _instance->getGPU();
  _graphicsQueue = _instance->getGraphicsQueue();
  _commandPool = _instance->getCommandPool();

  createDescriptorSetLayout();
  createGraphicsPipeline();
  createDescriptorPool();
}

void RenderAssets::cleanup() {
  _device.waitIdle();
  cleanupBufferMemory();
  cleanupImageMemory();
  cleanupDescriptorPool();
  cleanupDescriptorSetLayout();
  cleanupGraphicsPipeline();
  cleanupGraphicsPipelineLayout();
}

void RenderAssets::cleanupDescriptorPool() {
  _device.destroyDescriptorPool(_descriptorPool);
}

void RenderAssets::cleanupDescriptorSetLayout() {
  _device.destroyDescriptorSetLayout(_descriptorSetLayout);
}

void RenderAssets::cleanupGraphicsPipelineLayout() {
  _device.destroyPipelineLayout(_graphicsPipelineLayout);
}

void RenderAssets::cleanupGraphicsPipeline() {
  _device.destroyPipeline(_graphicsPipeline);
}

void RenderAssets::cleanupBufferMemory() {
  for (uint32_t i = 0; i < _bufferIndex; i++) {
    _device.destroyBuffer(_buffers.at(i));
    _device.freeMemory(_memories.at(i));
  }
}

void RenderAssets::cleanupImageMemory() {
  for (uint32_t i = 0; i < _imageIndex; i++) {
    _device.destroyImage(_images.at(i));
    _device.freeMemory(_imageMemories.at(i));
  }
}

vk::Pipeline RenderAssets::getGraphicsPipeline() const {
  return _graphicsPipeline;
}

vk::PipelineLayout RenderAssets::getGraphicsPipelineLayout() const {
  return _graphicsPipelineLayout;
}

vk::DescriptorPool RenderAssets::getDescriptorPool() const {
  return _descriptorPool;
}

vk::DescriptorSetLayout RenderAssets::getDescriptorSetLayout() const {
  return _descriptorSetLayout;
}

const vk::DescriptorSet* RenderAssets::getDescriptorSet() const {
  return _descriptorSets.data();
}

vk::Buffer RenderAssets::getBuffer(uint32_t index) const {
  return _buffers.at(index);
}

vk::Image RenderAssets::getImage(uint32_t index) const {
  return _images.at(index);
}

uint32_t RenderAssets::createImage(
    uint32_t width, 
    uint32_t height, 
    vk::Format format, 
    vk::ImageTiling tiling, 
    vk::ImageUsageFlags usage, 
    vk::MemoryPropertyFlags memoryProp) {
  vk::ImageCreateInfo createInfo;
  createInfo.setImageType(vk::ImageType::e2D)
            .setExtent(vk::Extent3D(
                  {width, height}, 
                  1
                  ))
            .setMipLevels(1)
            .setArrayLayers(1)
            .setFormat(format)
            .setTiling(tiling)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setUsage(usage)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setSharingMode(vk::SharingMode::eExclusive);

  _images[_imageIndex] = _device.createImage(createInfo);
  CHECK_NULL(_images[_imageIndex]);

  vk::MemoryRequirements memRequirements = _device.getImageMemoryRequirements(_images.at(_imageIndex));

  vk::MemoryAllocateInfo allocInfo;
  allocInfo.setAllocationSize(memRequirements.size)
           .setMemoryTypeIndex(findMemType(
                 memRequirements.memoryTypeBits, 
                 memoryProp, 
                 _gpu
                 ));

  _imageMemories[_imageIndex] = _device.allocateMemory(allocInfo);
  CHECK_NULL(_imageMemories[_imageIndex]);

  _device.bindImageMemory(_images[_imageIndex], _imageMemories[_imageIndex], 0);

  return _imageIndex++;
}

uint32_t RenderAssets::createBuffer(
    vk::DeviceSize size, 
    vk:: BufferUsageFlags usage, 
    vk::MemoryPropertyFlags memoryProp) {

  vk::BufferCreateInfo bufferInfo;
  bufferInfo.setSize(size)
            .setUsage(usage)
            .setSharingMode(vk::SharingMode::eExclusive);

  _buffers[_bufferIndex] = _device.createBuffer(bufferInfo);

  vk::MemoryRequirements memRequirement = _device.getBufferMemoryRequirements(_buffers.at(_bufferIndex));

  vk::MemoryAllocateInfo memoryInfo;
  memoryInfo.setAllocationSize(memRequirement.size)
            .setMemoryTypeIndex(findMemType(
                  memRequirement.memoryTypeBits, 
                  memoryProp, 
                  _gpu));

  _memories[_bufferIndex] = _device.allocateMemory(memoryInfo);

  _device.bindBufferMemory(_buffers.at(_bufferIndex), _memories.at(_bufferIndex), 0);

  return _bufferIndex++;
}

void RenderAssets::mapMemory(uint32_t index, vk::DeviceSize size, void** mem) {
  IF_THROW(
      _device.mapMemory(_memories.at(index), 0, size, vk::MemoryMapFlags(0), mem) != vk::Result::eSuccess, 
      trouble mapping memory
      );
}

// it's now specific for uniform buffers
void RenderAssets::updateDescriptorSets(uint32_t index) {
  std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _descriptorSetLayout);
  vk::DescriptorSetAllocateInfo allocInfo;
  allocInfo.setDescriptorPool(_descriptorPool)
           .setSetLayouts(layouts)
           .setDescriptorSetCount(static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));

  _descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  _descriptorSets = _device.allocateDescriptorSets(allocInfo);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vk::DescriptorBufferInfo bufferInfo;
    bufferInfo.setBuffer(_buffers[index])
              .setOffset(0 + i * sizeof(UniformBufferObject))
              .setRange(sizeof(UniformBufferObject));

    vk::WriteDescriptorSet descriptorWrite;
    descriptorWrite.setDstSet(_descriptorSets[i])
                   .setDstBinding(0)
                   .setDstArrayElement(0)
                   .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                   .setDescriptorCount(1)
                   .setBufferInfo(bufferInfo);

    _device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
  }
}

void RenderAssets::storeBuffer(vk::Buffer src, uint32_t dst, vk::DeviceSize size) {
  vk::BufferCopy copyRegion{};
  copyRegion.setDstOffset(0)
            .setSrcOffset(0)
            .setSize(size);
  
  vk::CommandBuffer cmdBuffer = beginSingleTimeCommands(_device, _instance->getCommandPool());

    cmdBuffer.copyBuffer(src, _buffers.at(dst), copyRegion);

  submitSingleTimeCommands(cmdBuffer, _instance->getGraphicsQueue(), _device, _instance->getCommandPool());
}

void RenderAssets::storeBufferToImage(vk::Buffer src, uint32_t dst, const uint32_t& width, const uint32_t& height) {
  vk::BufferImageCopy region;
  region.setBufferOffset(0)
        .setBufferRowLength(0)
        .setBufferImageHeight(0)
        .setImageSubresource(vk::ImageSubresourceLayers(
                 vk::ImageAspectFlagBits::eColor, 
                 0, 0, 1
                 ))
        .setImageOffset({0, 0, 0})
        .setImageExtent({width, height, 1});
  vk::CommandBuffer commandBuffer = beginSingleTimeCommands(_device, _instance->getCommandPool());
    commandBuffer.copyBufferToImage(src, _images.at(dst), vk::ImageLayout::eTransferDstOptimal, 1, &region);
  submitSingleTimeCommands(commandBuffer, _instance->getGraphicsQueue(), _device, _instance->getCommandPool());
}

void RenderAssets::createDescriptorSetLayout() {
  vk::DescriptorSetLayoutBinding uboDescLayoutBinding;
  uboDescLayoutBinding.setBinding(0)
                      .setStageFlags(vk::ShaderStageFlagBits::eVertex)
                      .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                      .setDescriptorCount(1);

  vk::DescriptorSetLayoutCreateInfo createInfo{};
  createInfo.setBindings(uboDescLayoutBinding)
            .setBindingCount(1);

  _descriptorSetLayout = _device.createDescriptorSetLayout(createInfo);
}

void RenderAssets::createGraphicsPipeline() {
  auto vertShaderCode = readBinaryFile("../glslShaders/build/vert.spv");
  auto fragShaderCode = readBinaryFile("../glslShaders/build/frag.spv");

  vk::ShaderModule vertShaderModule = createShaderModule(_device, vertShaderCode);
  vk::ShaderModule fragShaderModule = createShaderModule(_device, fragShaderCode);

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

  vk::Extent2D swapChainExtent = _instance->getSwapChainExtent();

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
  rasterizer.setFrontFace(vk::FrontFace::eCounterClockwise);
  rasterizer.setDepthBiasEnable(VK_FALSE);

  vk::PipelineMultisampleStateCreateInfo multisampling;
  multisampling.setSampleShadingEnable(VK_FALSE);
  multisampling.setRasterizationSamples(vk::SampleCountFlagBits::e1);

  vk::PipelineColorBlendAttachmentState colorBlendAttachment;
  colorBlendAttachment.setColorWriteMask(
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
  pipelineLayoutInfo.setSetLayouts(_descriptorSetLayout);

  _graphicsPipelineLayout = _device.createPipelineLayout(pipelineLayoutInfo);
  CHECK_NULL(_graphicsPipelineLayout);

  vk::RenderPass renderPass = _instance->getRenderPass();

  vk::GraphicsPipelineCreateInfo pipelineInfo;
  pipelineInfo.setStages(shaderStages);
  pipelineInfo.setPVertexInputState(&vertexInputInfo);
  pipelineInfo.setPInputAssemblyState(&inputAssembly);
  pipelineInfo.setPViewportState(&viewportState);
  pipelineInfo.setPRasterizationState(&rasterizer);
  pipelineInfo.setPMultisampleState(&multisampling);
  pipelineInfo.setPColorBlendState(&colorBlending);
  pipelineInfo.setPDynamicState(&dynamicState);
  pipelineInfo.setLayout(_graphicsPipelineLayout);
  pipelineInfo.setRenderPass(renderPass);
  pipelineInfo.setSubpass(0);

  vk::Result result;

  std::tie(result, _graphicsPipeline) = _device.createGraphicsPipeline(nullptr, pipelineInfo);

  IF_THROW(
      result != vk::Result::eSuccess,
      failed to create graphicsPipeline...
      );

  _device.destroyShaderModule(fragShaderModule);
  _device.destroyShaderModule(vertShaderModule);
}

void RenderAssets::createDescriptorPool() {
  vk::DescriptorPoolSize poolSize;
  poolSize.setType(vk::DescriptorType::eUniformBuffer)
          .setDescriptorCount(static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));

  vk::DescriptorPoolCreateInfo createInfo;
  createInfo.setPoolSizeCount(1)
            .setPoolSizes(poolSize)
            .setMaxSets(static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));

  _descriptorPool = _device.createDescriptorPool(createInfo);
}

