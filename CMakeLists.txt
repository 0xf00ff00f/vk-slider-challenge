cmake_minimum_required(VERSION 3.12)

project(carousel)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(3rdparty)

find_package(Vulkan REQUIRED)

set(DEMO_SOURCES
    main.cc
)

add_executable(demo
    ${DEMO_SOURCES}
)

target_link_libraries(demo
    Vulkan::Vulkan
    glfw
)
