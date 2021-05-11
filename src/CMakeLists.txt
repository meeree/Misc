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
set (FRACTAL_BINARY_NAME "fractal")
set (PACKING_BINARY_NAME "packing")
set (LIF_BINARY_NAME "lif")

if (CMAKE_BUILD_TYPE MATCHES "RELEASE")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOGLWRAP_DEBUG=0")
endif()

add_executable(${FRACTAL_BINARY_NAME} WIN32 ${FRACTAL_SOURCE} ${ICON})
add_executable(${PACKING_BINARY_NAME} WIN32 ${PACKING_SOURCE} ${ICON})
add_executable(${LIF_BINARY_NAME} WIN32 ${LIF_SOURCE} ${ICON})

set(WINDOWS_BINARIES ${FRACTAL_BINARY_NAME})
set(WINDOWS_BINARIES ${PACKING_BINARY_NAME})
set(WINDOWS_BINARIES ${LIF_BINARY_NAME})
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

