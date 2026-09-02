cmake_minimum_required(VERSION 3.22)
project(Mario LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    set(SFML_STATIC_LIBRARIES TRUE)
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/external/SFML-3.0.2/lib/cmake")
elseif(UNIX)
    set(SFML_STATIC_LIBRARIES FALSE)
    list(APPEND CMAKE_PREFIX_PATH "/home/linuxbrew/.linuxbrew")
endif()

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System Audio)

include_directories(include)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(${PROJECT_NAME} ${SRC_FILES})
set_property(TARGET ${PROJECT_NAME} PROPERTY WIN32_EXECUTABLE FALSE)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/external
)

add_subdirectory(external/Box2D-3.1.1)
target_link_libraries(${PROJECT_NAME} PRIVATE box2d::box2d)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        SFML::Audio
)

add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "${CMAKE_BINARY_DIR}/assets"
)

add_dependencies(${PROJECT_NAME} copy_assets)

target_compile_options(${PROJECT_NAME} PRIVATE -O2)