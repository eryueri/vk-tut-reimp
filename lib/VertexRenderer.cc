#include "VertexRenderer.hh"

#include <chrono>
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

void Renderer::init(VulkanInstance* instance, RenderAssets* assets) {
  _instance = instance;
  _assets = assets;

  _device = _instance->getLogicalDevice();
  _gpu = _instance->getGPU();

  allocateVertexBuffer();
  allocateIndexBuffer();
  allocateUniformBuffer();
  _assets->updateDescriptorSets(_uniformIndex.value());
}

void Renderer::drawFrame() {
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

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _assets->getGraphicsPipelineLayout(), 0, 1, &_assets->getDescriptorSet()[currentFrame], 0, nullptr);
    commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

    commandBuffer.endRenderPass();
  } _instance->getCommandBufferEnd();

  _instance->applyGraphicsQueue();

  _instance->applyPresentQueue(imageIndex);

  _instance->currentFrameInc();
}

void Renderer::createTextureImage() {
  int texWidth, texHeight, texChannels;
  stbi_uc* pixels = stbi_load("resources/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  vk::DeviceSize imageSize = texWidth * texHeight * 4;

  IF_THROW(
      !pixels,
      failed to load texture iamge...
      );

  vk::Buffer stagingBuffer;
  vk::DeviceMemory stagingMemory;
  std::tie(stagingBuffer, stagingMemory) = createStagingBuffer(imageSize, 
      _device, 
      _gpu);

  void* data;
  auto result = _device.mapMemory(stagingMemory, 0, imageSize, vk::MemoryMapFlags(0), &data);
    IF_THROW(
        result != vk::Result::eSuccess, 
        failed to map texture staging mem...
        );
    memcpy(data, pixels, imageSize);
  _device.unmapMemory(stagingMemory);

  stbi_image_free(pixels);

  _assets->createImage(
      texWidth, 
      texHeight, 
      vk::Format::eR8G8B8A8Srgb, 
      vk::ImageTiling::eOptimal, 
      vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, 
      vk::MemoryPropertyFlagBits::eDeviceLocal
      );
}

void Renderer::allocateVertexBuffer() {
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

void Renderer::allocateIndexBuffer() {
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

void Renderer::allocateUniformBuffer() {
  vk::DeviceSize bufferSize = sizeof(UniformBufferObject) * MAX_FRAMES_IN_FLIGHT;

  _uniformIndex = _assets->createBuffer(
      bufferSize, 
      vk::BufferUsageFlagBits::eUniformBuffer,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

  _assets->mapMemory(_uniformIndex.value(), bufferSize, &_data);
}

void Renderer::updateUniformBuffer(uint32_t currentFrame) {
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

void Renderer::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
  vk::CommandBuffer commandBuffer = beginSingleTimeCommands(_device, _instance->getCommandPool());
    vk::ImageMemoryBarrier barrier;
    barrier.setOldLayout(oldLayout)
           .setNewLayout(newLayout)
           .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
           .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
           .setImage(image)
           .setSubresourceRange(vk::ImageSubresourceRange(
                 vk::ImageAspectFlagBits::eColor, 
                 0, 1, 0, 1
                 ))
           .setSrcAccessMask(vk::AccessFlags(0))
           .setDstAccessMask(vk::AccessFlags(0));

    commandBuffer.pipelineBarrier(
        vk::PipelineStageFlags(0), vk::PipelineStageFlags(0), 
        vk::DependencyFlags(0), 
        0, nullptr, 
        0, nullptr, 
        1, &barrier
        );

  submitSingleTimeCommands(commandBuffer, _instance->getGraphicsQueue(), _device, _instance->getCommandPool());
}
