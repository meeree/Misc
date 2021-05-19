cmake_minimum_required(VERSION 2.8)

if (MSVC)
    link_libraries(opengl32)
else()
    link_libraries(GL)
endif()
link_libraries(glfw)
link_libraries(glad)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fopenmp")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DUSE_DEBUG_CONTEXT -g")

if (BUILD_SHARED_LIBS)
    link_libraries("${MATH_LIBRARY}")
endif()
if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set (LODEPNG_SOURCE "../deps/lodepng/lodepng.cpp")

file(GLOB FRACTAL_SOURCE "cpp/graphics.cpp" "cpp/oglwrap_example.cpp")
file(GLOB PACKING_SOURCE "cpp/graphics2.cpp" "cpp/oglwrap_example.cpp")
file(GLOB LIF_SOURCE "cpp/graphics_lif.cpp" "cpp/oglwrap_example.cpp")
file(GLOB FLUID_SOURCE "cpp/fluid.cpp" "cpp/oglwrap_example.cpp")
file(GLOB MINECRAFT_SOURCE "cpp_minecraft/graphics.cpp" "cpp_minecraft/oglwrap_example.cpp" ${LODEPNG_SOURCE})
file(GLOB MINECRAFT_INCLUDE "cpp_minecraft/*.h" "cpp_minecraft/*.hpp")
set (FRACTAL_BINARY_NAME "fractal")
set (PACKING_BINARY_NAME "packing")
set (LIF_BINARY_NAME "lif")
set (FLUID_BINARY_NAME "fluid")
set (MINECRAFT_BINARY_NAME "minecraft")

if (CMAKE_BUILD_TYPE MATCHES "RELEASE")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOGLWRAP_DEBUG=0")
endif()

add_executable(${FRACTAL_BINARY_NAME} ${FRACTAL_SOURCE} ${ICON})
add_executable(${PACKING_BINARY_NAME} ${PACKING_SOURCE} ${ICON})
add_executable(${LIF_BINARY_NAME} ${LIF_SOURCE} ${ICON})
add_executable(${FLUID_BINARY_NAME} ${FLUID_SOURCE} ${ICON})
add_executable(${MINECRAFT_BINARY_NAME} ${MINECRAFT_SOURCE} ${MINECRAFT_INCLUDE} ${ICON})

set(WINDOWS_BINARIES ${FRACTAL_BINARY_NAME} 
${PACKING_BINARY_NAME}
${LIF_BINARY_NAME}
${FLUID_BINARY_NAME}
${MINECRAFT_BINARY_NAME})
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

