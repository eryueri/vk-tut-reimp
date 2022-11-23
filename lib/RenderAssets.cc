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
  _graphicsQueue = _instance->getGraphicsQueue();
  _commandPool = _instance->getCommandPool();

  createGraphicsPipeline();
}

void RenderAssets::cleanup() {
  cleanupBufferMemory();
  cleanupGraphicsPipeline();
}

void RenderAssets::cleanupGraphicsPipeline() {
  _device.destroyPipeline(_graphicsPipeline);
}

void RenderAssets::cleanupBufferMemory() {
  for (uint32_t i = 0; i < _index; i++) {
    try{
      _memoriesMapped.at(i);
    } catch (std::out_of_range& e) {
      _device.destroyBuffer(_buffers.at(i));
      _device.freeMemory(_memories.at(i));
      // _buffers.erase(i);
      // _memories.erase(i);
    }
    _device.unmapMemory(_memories.at(i));
    _device.destroyBuffer(_buffers.at(i));
    _device.freeMemory(_memories.at(i));
    // _buffers.erase(i);
    // _memories.erase(i);
    // _memoriesMapped.erase(i);
  }
}

void RenderAssets::setInstance(VulkanInstance* instance) {
  _instance = instance;
}

vk::Pipeline RenderAssets::getGraphicsPipeline() const {
  return _graphicsPipeline;
}

vk::PipelineLayout RenderAssets::getGraphicsPipelineLayout() const {
  return _graphicsPipelineLayout;
}

vk::Buffer RenderAssets::getBuffer(uint32_t index) const {
  return _buffers.at(index);
}

void* RenderAssets::getMemoryMap(uint32_t index) const {
  return _memoriesMapped.at(index);
}

uint32_t RenderAssets::createBuffer(vk::DeviceSize size, 
    vk:: BufferUsageFlags usage, 
    vk::MemoryPropertyFlags memoryProp) {

  vk::BufferCreateInfo bufferInfo;
  bufferInfo.setSize(size)
            .setUsage(usage)
            .setSharingMode(vk::SharingMode::eExclusive);

  _buffers.at(_index) = _device.createBuffer(bufferInfo);

  vk::MemoryRequirements memRequirement = _device.getBufferMemoryRequirements(_buffers.at(_index));

  vk::MemoryAllocateInfo memoryInfo;
  memoryInfo.setAllocationSize(memRequirement.size)
            .setMemoryTypeIndex(findMemType(
                  memRequirement.memoryTypeBits, 
                  memoryProp, 
                  _gpu));

  _memories.at(_index) = _device.allocateMemory(memoryInfo);

  _device.bindBufferMemory(_buffers.at(_index), _memories.at(_index), 0);

  return _index++;
}

void RenderAssets::storeBuffer(vk::Buffer src, uint32_t dst, vk::DeviceSize size) {
  vk::CommandBufferAllocateInfo allocInfo;
  allocInfo.setCommandBufferCount(1)
           .setCommandPool(_commandPool)
           .setLevel(vk::CommandBufferLevel::ePrimary);

  vk::CommandBuffer cmdBuffer = _device.allocateCommandBuffers(allocInfo)[0];

  vk::BufferCopy copyRegion{};
  copyRegion.setDstOffset(0)
            .setSrcOffset(0)
            .setSize(size);

  vk::CommandBufferBeginInfo beginInfo;
  beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

  cmdBuffer.begin(beginInfo);
    cmdBuffer.copyBuffer(src, _buffers.at(dst), copyRegion);
  cmdBuffer.end();

  vk::SubmitInfo submitInfo;
  submitInfo.setCommandBuffers(cmdBuffer);

  _graphicsQueue.submit(submitInfo);
  _graphicsQueue.waitIdle();
  _device.freeCommandBuffers(_commandPool, 1, &cmdBuffer);
}

void RenderAssets::createDescriptorSetLayout() {
  vk::DescriptorSetLayoutBinding uboDescLayoutBinding;
  uboDescLayoutBinding.setBinding(0)
                      .setStageFlags(vk::ShaderStageFlagBits::eVertex)
                      .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                      .setDescriptorCount(1);

  vk::DescriptorSetLayoutCreateInfo createInfo{};
  createInfo.setBindings(uboDescLayoutBinding);

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
  pipelineInfo.setRenderPass(_instance->getRenderPass());
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

void RenderAssets::createDescriptorSets() {
  std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _descriptorSetLayout);
  vk::DescriptorSetAllocateInfo allocInfo;
  allocInfo.setDescriptorPool(_descriptorPool)
           .setSetLayouts(layouts)
           .setDescriptorSetCount(static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));

  descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  descriptorSets = _device.allocateDescriptorSets(allocInfo);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vk::DescriptorBufferInfo bufferInfo;
    // bufferInfo.setBuffer(uniformBuffers[i])
    //           .setOffset(0)
    //           .setRange(sizeof(UniformBufferObj));

    vk::WriteDescriptorSet descriptorWrite;
    descriptorWrite.setDstSet(descriptorSets[i])
                   .setDstBinding(0)
                   .setDstArrayElement(0)
                   .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                   .setDescriptorCount(1)
                   .setBufferInfo(bufferInfo);

    _device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
  }
}
