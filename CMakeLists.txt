cmake_minimum_required(VERSION 3.5)

project(BisonChatProject)



# Find the GTK library
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# Add the GTK include directory to the include path
include_directories(${GTK3_INCLUDE_DIRS})

# Add the GTK library to the linker flags
link_directories(${GTK3_LIBRARY_DIRS})
link_libraries(${GTK3_LIBRARIES})



# Find the libcurl library
find_package(CURL REQUIRED)

# Find the nlohmann_json library
find_package(nlohmann_json REQUIRED)

# Copy files from the res directory to the build/out/res directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/res DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/out)

# Add all source files in the src directory
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

# Create an executable from the source files
add_executable(BisonChat ${SOURCES} ${HEADERS})

# Link against the GTK library
target_link_libraries(BisonChat ${GTK3_LIBRARIES} ${GOBJECT_LIBRARIES})

# Link the libcurl library to your executable
target_link_libraries(BisonChat CURL::libcurl)

# Link the nlohmann_json library to your executable
target_link_libraries(BisonChat nlohmann_json::nlohmann_json)

# Include the GTK headers
target_include_directories(BisonChat PUBLIC ${GTK3_INCLUDE_DIRS})

# Set the working directory to the build directory
set_target_properties(BisonChat PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Set the output directory for the executable
set_target_properties(BisonChat PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/out)