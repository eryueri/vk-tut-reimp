#include "app.hh"
#include <iostream>

int main() {
  myWindow::MainWindow* window = myWindow::MainWindow::getInstance();

  try {
    window->run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
