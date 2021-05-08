cmake_minimum_required(VERSION 3.1)
project(matryoshka)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# libigl
option(LIBIGL_USE_STATIC_LIBRARY "Use libigl as static library" ON)
option(LIBIGL_WITH_CGAL             "Use CGAL"           ON)
option(LIBIGL_WITH_MATLAB           "Use Matlab"         ON)
option(LIBIGL_WITH_OPENGL           "Use OpenGL"         ON)
option(LIBIGL_WITH_OPENGL_GLFW      "Use GLFW"           ON)
option(LIBIGL_WITH_PNG              "Use PNG"            ON)
find_package(LIBIGL REQUIRED)
#if(LIBIGL_WITH_CGAL) # Do not remove or move this block, the cgal build system fails without it
#  find_package(CGAL REQUIRED COMPONENTS Core)
#  set(CGAL_DONT_OVERRIDE_CMAKE_FLAGS TRUE CACHE BOOL "CGAL's CMAKE Setup is super annoying ")
#  include(${CGAL_USE_FILE})
#endif()

file(GLOB SRCFILES  
  ${PROJECT_SOURCE_DIR}/include/igl/*.cpp
  ${PROJECT_SOURCE_DIR}/include/igl/opengl/*.cpp
  ${PROJECT_SOURCE_DIR}/include/igl/copyleft/*.cpp
  ${PROJECT_SOURCE_DIR}/include/igl/copyleft/cgal/*.cpp
  )

add_executable(maximize_nesting_scale maximize_nesting_scale.cpp ${SRCFILES})
target_link_libraries(maximize_nesting_scale igl::core igl::png igl::opengl igl::opengl_glfw igl::cgal)

add_executable(profile profile.cpp ${SRCFILES})
target_link_libraries(profile igl::core igl::png igl::opengl igl::opengl_glfw igl::cgal)

add_executable(modeler modeler.cpp ${SRCFILES})
target_link_libraries(modeler igl::core igl::png igl::opengl igl::opengl_glfw igl::cgal)

add_executable(matryoshka matryoshka.cpp ${SRCFILES})
target_link_libraries(matryoshka igl::core igl::png igl::opengl igl::opengl_glfw igl::cgal)

add_executable(matryoshkaModular matryoshkaModular.cpp ${SRCFILES})
target_link_libraries(matryoshkaModular igl::core igl::png igl::opengl igl::opengl_glfw igl::cgal)
