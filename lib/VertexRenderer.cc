#include "VertexRenderer.hh"

#include <chrono>
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>

#include "VulkanInstance.hh"
#include "RenderAssets.hh"
#include "Structs.hh"
#include "VkUtils.hh"
#include "Macros.hh"

const std::vector<Vertex> vertices = {
  { { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
  { {  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } },
  { {  0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f } },
  { { -0.5f,  0.5f }, { 1.0f, 0.0f, 1.0f } },
};

const std::vector<uint16_t> indices = {
  0, 1, 2, 2, 3, 0
};

void VertexRenderer::init(VulkanInstance* instance, RenderAssets* assets) {
  _instance = instance;
  _assets = assets;

  _device = _instance->getLogicalDevice();
  _gpu = _instance->getGPU();

  allocateVertexBuffer();
  allocateIndexBuffer();
  allocateUniformBuffer();
  updateDescriptorSets();
}

void VertexRenderer::drawFrame() {
  uint32_t currentFrame = _instance->getCurrentFrame();
  uint32_t imageIndex = _instance->acquireImage();

  _instance->waitForFence();

  updateUniformBuffer(currentFrame);

  vk::CommandBuffer commandBuffer = _instance->getCommandBufferBegin(); {
    vk::Extent2D swapChainExtent = _instance->getSwapChainExtent();

    vk::RenderPassBeginInfo renderPassInfo;
    renderPassInfo.setRenderPass(_instance->getRenderPass());
    renderPassInfo.setFramebuffer(_instance->getFramebuffer(imageIndex));
    renderPassInfo.setRenderArea(
        vk::Rect2D(
            {0, 0},
            swapChainExtent
          )
        );
    vk::ClearValue clearColor(vk::ClearColorValue().setFloat32({0.0f, 0.0f, 0.0f, 0.5f}));
    renderPassInfo.setClearValues(clearColor);

    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _assets->getGraphicsPipeline());

    std::vector<vk::Buffer> vertexBuffers = { _assets->getBuffer(_vertexIndex.value()) };
    vk::Buffer indexBuffer = _assets->getBuffer(_indexIndex.value());
    std::vector<vk::DeviceSize> offsets = { 0 };

    commandBuffer.bindVertexBuffers(0, 1, vertexBuffers.data(), offsets.data());
    commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint16);

    vk::Viewport viewport;
    viewport.setX(0.0f);
    viewport.setY(0.0f);
    viewport.setWidth(static_cast<float>(swapChainExtent.width));
    viewport.setHeight(static_cast<float>(swapChainExtent.height));
    viewport.setMinDepth(0.0f);
    viewport.setMaxDepth(1.0f);
    
    commandBuffer.setViewport(0, 1, &viewport);

    vk::Rect2D scissor;
    scissor.setOffset(vk::Offset2D(0, 0));
    scissor.setExtent(swapChainExtent);

    commandBuffer.setScissor(0, 1, &scissor);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _assets->getGraphicsPipelineLayout(), 0, 1, &_descriptorSets[currentFrame], 0, nullptr);
    commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

    commandBuffer.endRenderPass();
  } _instance->getCommandBufferEnd();

  _instance->applyGraphicsQueue();

  _instance->applyPresentQueue(imageIndex);

  _instance->currentFrameInc();
}

void VertexRenderer::allocateVertexBuffer() {
  vk::DeviceSize bufferSize = sizeof(Vertex) * vertices.size();
  vk::Buffer stagingBuffer;
  vk::DeviceMemory stagingMemory;

  std::tie(stagingBuffer, stagingMemory) = createStagingBuffer(
      bufferSize,
      _device, 
      _gpu
      );

  _vertexIndex = _assets->createBuffer(
      bufferSize, 
      vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, 
      vk::MemoryPropertyFlagBits::eDeviceLocal);

  void* data;
  IF_THROW(
      _device.mapMemory(stagingMemory, 0, bufferSize, vk::MemoryMapFlags(0), &data) != vk::Result::eSuccess,
      failed to map memory
      );
    memcpy(data, vertices.data(), bufferSize);
  _device.unmapMemory(stagingMemory);

  _assets->storeBuffer(stagingBuffer, _vertexIndex.value(), bufferSize);

  _device.destroyBuffer(stagingBuffer);
  _device.freeMemory(stagingMemory);
}

void VertexRenderer::allocateIndexBuffer() {
  vk::DeviceSize bufferSize = sizeof(uint16_t) * indices.size();
  vk::Buffer stagingBuffer;
  vk::DeviceMemory stagingMemory;

  std::tie(stagingBuffer, stagingMemory) = createStagingBuffer(
      bufferSize,
      _device, 
      _gpu
      );

  _indexIndex = _assets->createBuffer(
      bufferSize, 
      vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, 
      vk::MemoryPropertyFlagBits::eDeviceLocal);

  void* data;
  IF_THROW(
      _device.mapMemory(stagingMemory, 0, bufferSize, vk::MemoryMapFlags(0), &data) != vk::Result::eSuccess,
      failed to map memory
      );
    memcpy(data, indices.data(), bufferSize);
  _device.unmapMemory(stagingMemory);

  _assets->storeBuffer(stagingBuffer, _indexIndex.value(), bufferSize);

  _device.destroyBuffer(stagingBuffer);
  _device.freeMemory(stagingMemory);
}

void VertexRenderer::allocateUniformBuffer() {
  vk::DeviceSize bufferSize = sizeof(UniformBufferObject) * MAX_FRAMES_IN_FLIGHT;

  _uniformIndex = _assets->createBuffer(
      bufferSize, 
      vk::BufferUsageFlagBits::eUniformBuffer,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

  _assets->mapMemory(_uniformIndex.value(), bufferSize, &_data);
}

void VertexRenderer::updateDescriptorSets() {
  std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _assets->getDescriptorSetLayout());
  vk::DescriptorSetAllocateInfo allocInfo;
  allocInfo.setDescriptorPool(_assets->getDescriptorPool())
           .setSetLayouts(layouts)
           .setDescriptorSetCount(static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));

  _descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  _descriptorSets = _device.allocateDescriptorSets(allocInfo);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vk::DescriptorBufferInfo bufferInfo;
    bufferInfo.setBuffer(_assets->getBuffer(_uniformIndex.value()))
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

void VertexRenderer::updateUniformBuffer(uint32_t currentFrame) {
  static auto startTime = std::chrono::high_resolution_clock::now();
  auto currentTime = std::chrono::high_resolution_clock::now();

  vk::Extent2D swapChainExtent = _instance->getSwapChainExtent();

  float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
  UniformBufferObject ubo{};
  ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float) swapChainExtent.height, 0.1f, 10.0f);
  ubo.proj[1][1] *= -1;
  memcpy((void*)((UniformBufferObject*)_data + currentFrame), &ubo, sizeof(ubo));
}
