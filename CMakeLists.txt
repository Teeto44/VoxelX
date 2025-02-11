cmake_minimum_required(VERSION 3.11)
project(VoxelX)
set(RAYLIB_VERSION 5.5)

# Define the source directory
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Adding Raylib (This came straight from Raylib examples, will probably change it in future)
find_package(raylib ${RAYLIB_VERSION} QUIET)
if (NOT raylib_FOUND) # If there's none, fetch and build Raylib
		include(FetchContent)
		FetchContent_Declare(
						raylib
						DOWNLOAD_EXTRACT_TIMESTAMP OFF
						URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
						)
		FetchContent_GetProperties(raylib)
		if (NOT raylib_POPULATED) # Have we downloaded Raylib yet?
				set(FETCHCONTENT_QUIET NO)
				FetchContent_MakeAvailable(raylib)
				set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # Don't build the supplied examples
		endif ()
endif ()

# Adding CRLImGui (rLImGui + Cimgui + Imgui) -- Please read README.txt in the external/crlimgui folder
set(CRLIMGUI_LIB "${CMAKE_SOURCE_DIR}/external/crlimgui/library/crlimgui.lib")
set(CRLIMGUI_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/crlimgui/include")

set(TINYCTHREAD_SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/tinycthread/library")
set(TINYCTHREAD_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/tinycthread/include")

# Use file(GLOB_RECURSE ...) to get all .c, .cpp, .h, and .hpp files in the src directory
file(GLOB_RECURSE SOURCES "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp")
file(GLOB_RECURSE HEADERS "${SRC_DIR}/*.h" "${SRC_DIR}/*.hpp")

# Debugging: Print the list of source files
message(STATUS "Source files: ${SOURCES}")
message(STATUS "Header files: ${HEADERS}")

# Add the executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${TINYCTHREAD_SOURCE_DIR})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
target_link_libraries(${PROJECT_NAME} PRIVATE ${CRLIMGUI_LIB})
target_include_directories(${PROJECT_NAME} PRIVATE ${HEADERS}
													 ${CRLIMGUI_INCLUDE_DIR}
													 ${TINYCTHREAD_INCLUDE_DIR})

# Setting ASSETS_PATH
target_compile_definitions(${PROJECT_NAME} PUBLIC RES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/res/")
#target_compile_definitions(${PROJECT_NAME} PUBLIC RES_PATH="./res") -- This is to be used when building for release
