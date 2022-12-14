cmake_minimum_required(VERSION 3.8)

project(my_vulkan_tut_reimplement)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(IS_DEBUG_BUILD CMAKE_BUILD_TYPE STREQUAL "Debug")

if (${IS_DEBUG_BUILD})
    add_compile_definitions(DEBUG)
endif()

include_directories(include/)
include_directories(deps/)

aux_source_directory(./lib LIB_FILES)
aux_source_directory(./src SOURCE_FILES)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

add_library(vulkan-reimp ${LIB_FILES})

add_executable(reimp-execute ${SOURCE_FILES})

target_link_libraries(reimp-execute
  PRIVATE
  vulkan-reimp
  glfw
  vulkan
)
