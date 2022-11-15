#include "window.hh"

int main() {
  Window::MainWindow* window = Window::MainWindow::getInstance();

  try {
    window->run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
