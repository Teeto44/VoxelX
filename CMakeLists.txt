cmake_minimum_required(VERSION 3.11)
project(VoxelX)
set(RAYLIB_VERSION 5.5)

# Define the source directory
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")

# Adding CRLImGui (rLImGui + Cimgui + Imgui) -- Please read README.txt in the external/crlimgui folder
set(CRLIMGUI_LIB "${CMAKE_SOURCE_DIR}/external/crlimgui/library/crlimgui.lib")
set(CRLIMGUI_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/crlimgui/include")

set(TINYCTHREAD_SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/tinycthread/library")
set(TINYCTHREAD_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/tinycthread/include")
set(TINYCTHREAD_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/tinycthread/include")

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

# Function to collect all subdirectories in src/
function(collect_subdirectories BASE_DIR RESULT)
		file(GLOB CHILDREN RELATIVE ${BASE_DIR} ${BASE_DIR}/*)
		set(DIRS "")
		foreach(CHILD ${CHILDREN})
				if(IS_DIRECTORY ${BASE_DIR}/${CHILD})
						list(APPEND DIRS ${BASE_DIR}/${CHILD})
						collect_subdirectories(${BASE_DIR}/${CHILD} SUBDIRS)
						list(APPEND DIRS ${SUBDIRS})
				endif()
		endforeach()
		set(${RESULT} ${DIRS} PARENT_SCOPE)
endfunction()

# Use file(GLOB_RECURSE ...) to get all .c, .cpp, .h, and .hpp files in the src directory
file(GLOB_RECURSE SOURCES "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp")
file(GLOB_RECURSE HEADERS "${SRC_DIR}/*.h" "${SRC_DIR}/*.hpp")

# Add the executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${TINYCTHREAD_SOURCE_DIR}) # Tinycthread needs to be built with the project

# find all subdirectories in src/
collect_subdirectories(${SRC_DIR} SUBDIRS)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE raylib ${CRLIMGUI_LIB})

target_include_directories(${PROJECT_NAME} PRIVATE ${CRLIMGUI_INCLUDE_DIR}
													 ${TINYCTHREAD_INCLUDE_DIR} ${SRC_DIR} ${SUBDIRS})

# Setting ASSETS_PATH
target_compile_definitions(${PROJECT_NAME} PUBLIC RES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/res/")
#target_compile_definitions(${PROJECT_NAME} PUBLIC RES_PATH="./res") -- This is to be used when building for release