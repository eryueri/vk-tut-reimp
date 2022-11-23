#include "Structs.hh"

bool QueueFamilyIndices::isComplete() {
  return graphicsFamily.has_value() && presentFamily.has_value();
}

vk::VertexInputBindingDescription Vertex::getBindingDescription() {
  vk::VertexInputBindingDescription description{};
  description.setBinding(0)
             .setStride(sizeof(Vertex))
             .setInputRate(vk::VertexInputRate::eVertex);

  return description;
}

std::array<vk::VertexInputAttributeDescription, 2> Vertex::getAttributeDescriptions() {
  std::array<vk::VertexInputAttributeDescription, 2> descriptions;
  descriptions[0].setBinding(0)
                 .setLocation(0)
                 .setFormat(vk::Format::eR32G32Sfloat)
                 .setOffset(offsetof(Vertex, pos));
  descriptions[1].setBinding(0)
                 .setLocation(1)
                 .setFormat(vk::Format::eR32G32B32Sfloat)
                 .setOffset(offsetof(Vertex, color));

  return descriptions;
}
