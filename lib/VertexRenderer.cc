#include "VertexRenderer.hh"


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
}

void VertexRenderer::drawFrame() {
  uint32_t currentFrame = _instance->getCurrentFrame();
  uint32_t imageIndex = _instance->acquireImage();

  _instance->waitForFence();

  vk::CommandBuffer commandBuffer = _instance->getCommandBufferBegin();
  {
    vk::Extent2D swapChainExtent = _instance->getSwapChainExtent();

    vk::RenderPassBeginInfo renderPassInfo;
    renderPassInfo.setRenderPass(_instance->getRenderPass());
    renderPassInfo.setFramebuffer(_instance->getFramebuffer(imageIndex));
    renderPassInfo.setRenderArea( // SUS
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

    // commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _assets->getGraphicsPipelineLayout(), 0, 1, &descriptorSets[currentFrame], 0, nullptr);
    commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

    commandBuffer.endRenderPass();
  }
  _instance->getCommandBufferEnd();

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
    memcpy(data, vertices.data(), bufferSize);
  _device.unmapMemory(stagingMemory);

  _assets->storeBuffer(stagingBuffer, _indexIndex.value(), bufferSize);

  _device.destroyBuffer(stagingBuffer);
  _device.freeMemory(stagingMemory);
}
