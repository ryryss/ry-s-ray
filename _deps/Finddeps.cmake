# Chinese developer do not use fetch feature cause it's as slow as a turtle on cn network
# include(FetchContent)
# FetchContent_Declare
# FetchContent_Populate

set(DEPS_DIR "${PROJECT_SOURCE_DIR}/_deps")
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
    INTERFACE_INCLUDE_DIRECTORIES "${GLFW_DIR}/include"
)