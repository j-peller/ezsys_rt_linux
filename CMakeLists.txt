cmake_minimum_required(VERSION 3.10)
project(RPISignal LANGUAGES C)

# Optionally enforce standards
set(CMAKE_C_STANDARD 11)

# Optionally set the default build type if not provided by the user
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g -Wall")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")

# Collect all .cpp files in the src/ directory (you can add other extensions as needed)
file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.c")

#set(CMAKE_EXE_LINKER_FLAGS "-static")

# Create an executable target using the collected source files
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE pthread gpiod)

# Include the src/ directory in the include search paths if necessary
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
