set(PROJECT_NAME lab5)

cmake_minimum_required(VERSION 2.8)
project(${PROJECT_NAME})

find_package(OpenGL)

find_package(GLEW REQUIRED)

if(APPLE)
   find_library(GLFW_LIB glfw3)

   find_library(COCOA_LIB Cocoa)
   find_library(IOKIT_LIB IOKit)
   find_library(COREVIDEO_LIB CoreVideo)
   set(GLFW_DEPENDENT_LIBS ${COCOA_LIB} ${IOKIT_LIB} ${COREVIDEO_LIB})

   set(PLATFORM_LIBS ${GLFW_DEPENDENT_LIBS})
elseif(WIN32)
   set(GLFW_LIB glfw3)
   set(PLATFORM_LIBS glew32)
endif()

add_executable(${PROJECT_NAME} main.cpp GLSL.cpp tiny_obj_loader.cc)

target_link_libraries(${PROJECT_NAME} ${OPENGL_gl_LIBRARY} ${GLFW_LIB} ${GLEW_LIBRARY} ${PLATFORM_LIBS})
