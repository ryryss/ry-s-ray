cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project("ry-s-ray" LANGUAGES CXX)

option(DEBUG "Debug mode" OFF)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Please select another build directory")
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/_deps")
set(3rd_libs "")
find_package(deps REQUIRED)
find_package(OpenGL REQUIRED)

MACRO(GET_SUB_DIR_WITH_RECURSIVE src res)
    FILE(GLOB_RECURSE new_list ${src} ${src}/*)
    SET(dir_list "")
    FOREACH(file_path ${new_list})
        GET_FILENAME_COMPONENT(dir_path ${file_path} PATH)
        SET(dir_list ${dir_list} ${dir_path})
    ENDFOREACH()
    LIST(REMOVE_DUPLICATES dir_list)
    SET(${res} ${dir_list})
ENDMACRO()

GET_SUB_DIR_WITH_RECURSIVE("${CMAKE_SOURCE_DIR}/src" all_dir)
GET_SUB_DIR_WITH_RECURSIVE("${CMAKE_SOURCE_DIR}/include" all_dir)
    
include_directories(
    ${all_dir}
    ${PROJECT_SOURCE_DIR}/_deps
)

file(GLOB_RECURSE SRCS ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE HEADS ${CMAKE_SOURCE_DIR}/include/*.h)

add_executable(${PROJECT_NAME}
    ${SRCS}
    ${HEADS}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    tinygltf
    glm
    glfw
    OpenGL::GL
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADS})

# after build
set(EXE_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(GLFW_DLL "${GLFW_BUILD_DIR}/src/Release/glfw3.dll")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GLFW_DLL}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMENT "Copying glfw3.dll to output directory"
)

# print all variable for debug
if(DEBUG)
    get_cmake_property(_variableNames VARIABLES)
    list(SORT _variableNames)
    foreach(_variableNames ${_variableNames})
        message(STATUS "${_variableNames}=${${_variableNames}}")
    endforeach()
endif()