#include "app.hh"

int main() {
  HelloTriangle::Application* app;

  app = HelloTriangle::Application::getInstance();

  try {
    app->run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
