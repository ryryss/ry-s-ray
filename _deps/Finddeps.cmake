# Chinese developer do not use fetch feature cause it's as slow as a turtle on cn network
# include(FetchContent)
# FetchContent_Declare
# FetchContent_Populate

#90% code of this file are from ChatGPT

set(DEPS_DIR "${PROJECT_SOURCE_DIR}/_deps")
set(DEPS "")
# =====================
# imgui
# =====================
option(ENABLE_IMGUI "Is enable imgui" OFF)
if(ENABLE_IMGUI)
    add_definitions(-DUSE_IMGUI) 
    if(NOT EXISTS "${IMGUI_DIR}/.git")
        message(STATUS "Cloning imgui...")
        execute_process(
            COMMAND git clone --depth 1 https://github.com/ocornut/imgui.git -b master ${IMGUI_DIR}
            RESULT_VARIABLE res
        )
        if(NOT res EQUAL 0)
            message(FATAL_ERROR "Failed to clone imgui")
        endif()
    endif()
    message(STATUS "imgui at ${IMGUI_DIR}")
    add_library(imgui INTERFACE)
    target_include_directories(imgui INTERFACE ${IMGUI_DIR})
    set(IMGUI_SRC
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
    list(APPEND DEPS imgui)
endif()

# =====================
# tinygltf
# =====================
set(TINYGLTF_DIR "${DEPS_DIR}/tinygltf")
if(NOT EXISTS "${TINYGLTF_DIR}/.git")
    message(STATUS "Cloning tinygltf...")
    execute_process(
        COMMAND git clone --depth 1 https://github.com/syoyo/tinygltf.git -b release ${TINYGLTF_DIR}
        RESULT_VARIABLE res
    )
    if(NOT res EQUAL 0)
        message(FATAL_ERROR "Failed to clone tinygltf")
    endif()
endif()
message(STATUS "tinygltf at ${TINYGLTF_DIR}")
add_library(tinygltf INTERFACE)
target_include_directories(tinygltf INTERFACE ${TINYGLTF_DIR})
list(APPEND DEPS tinygltf)

# =====================
# glm
# =====================
set(GLM_DIR "${DEPS_DIR}/glm")
if(NOT EXISTS "${GLM_DIR}/.git")
    message(STATUS "Cloning glm...")
    execute_process(
        COMMAND git clone --depth 1 https://github.com/g-truc/glm.git -b master ${GLM_DIR}
        RESULT_VARIABLE res
    )
    if(NOT res EQUAL 0)
        message(FATAL_ERROR "Failed to clone glm")
    endif()
endif()
message(STATUS "glm at ${GLM_DIR}")
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ${GLM_DIR})
list(APPEND DEPS glm)

# =====================
# glfw
# =====================
set(GLFW_DIR "${DEPS_DIR}/glfw")
set(GLFW_BUILD_DIR "${GLFW_DIR}/build")
if(NOT EXISTS "${GLFW_DIR}/.git")
    message(STATUS "Cloning glfw...")
    execute_process(
        COMMAND git clone --depth 1 https://github.com/glfw/glfw.git -b master ${GLFW_DIR}
        RESULT_VARIABLE res
    )
    if(NOT res EQUAL 0)
        message(FATAL_ERROR "Failed to clone glfw")
    endif()
endif()
message(STATUS "glfw at ${GLFW_DIR}")
list(APPEND DEPS glfw)

# build glfw
if(NOT EXISTS "${GLFW_BUILD_DIR}/CMakeCache.txt")
    file(MAKE_DIRECTORY ${GLFW_BUILD_DIR})
    execute_process(
        COMMAND ${CMAKE_COMMAND} -S ${GLFW_DIR} -B ${GLFW_BUILD_DIR} -DBUILD_SHARED_LIBS=ON
            -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF
        RESULT_VARIABLE res
    )
    if(NOT res EQUAL 0)
        message(FATAL_ERROR "Failed to configure glfw")
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ${GLFW_BUILD_DIR} --config Release
        RESULT_VARIABLE res
    )
    if(NOT res EQUAL 0)
        message(FATAL_ERROR "Failed to build glfw")
    endif()
endif()

# add glfw target
add_library(glfw STATIC IMPORTED)
set_target_properties(glfw PROPERTIES
    IMPORTED_LOCATION "${GLFW_BUILD_DIR}/src/Release/glfw3dll.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${GLFW_DIR}/include")

# =====================
# cuda
# =====================
execute_process(
    COMMAND nvidia-smi --query-gpu=compute_cap --format=csv,noheader
    OUTPUT_VARIABLE CUDA_CAP
    OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REPLACE "." "" CUDA_ARCH ${CUDA_CAP})
set(CMAKE_CUDA_ARCHITECTURES ${CUDA_ARCH})

option(ENABLE_CUDA "Is enable cuda" ON)
find_package(CUDAToolkit)
if(ENABLE_CUDA AND CUDAToolkit_FOUND)
    enable_language(CUDA)
    add_definitions(-DUSE_CUDA)
    set(CUDA_LIB CUDA::cudart)
    file(GLOB_RECURSE CU_SRCS ${CMAKE_SOURCE_DIR}/source/*.cu)
else()
    message(STATUS "CUDA not found or USE_CUDA is off, building CPU-only version")
    set(CUDA_LIB "")
    set(CU_SRCS "")
endif()