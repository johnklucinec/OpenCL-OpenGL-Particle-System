cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0074 NEW)
project(OpenGLApp VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(OpenGL_GL_PREFERENCE LEGACY)
find_package(OpenGL REQUIRED)

# ── GLFW ──────────────────────────────────────────────────────────────────────
set(GLFW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/glfw)
set(GLFW_SOURCES
    ${GLFW_DIR}/src/context.c
    ${GLFW_DIR}/src/init.c
    ${GLFW_DIR}/src/input.c
    ${GLFW_DIR}/src/monitor.c
    ${GLFW_DIR}/src/platform.c
    ${GLFW_DIR}/src/vulkan.c
    ${GLFW_DIR}/src/window.c
    ${GLFW_DIR}/src/egl_context.c
    ${GLFW_DIR}/src/osmesa_context.c
    ${GLFW_DIR}/src/null_init.c
    ${GLFW_DIR}/src/null_monitor.c
    ${GLFW_DIR}/src/null_window.c
    ${GLFW_DIR}/src/null_joystick.c
)

if(WIN32)
    list(APPEND GLFW_SOURCES
        ${GLFW_DIR}/src/win32_init.c
        ${GLFW_DIR}/src/win32_joystick.c
        ${GLFW_DIR}/src/win32_module.c
        ${GLFW_DIR}/src/win32_monitor.c
        ${GLFW_DIR}/src/win32_thread.c
        ${GLFW_DIR}/src/win32_time.c
        ${GLFW_DIR}/src/win32_window.c
        ${GLFW_DIR}/src/wgl_context.c
    )
    add_definitions(-D_GLFW_WIN32 -D_CRT_SECURE_NO_WARNINGS)
elseif(APPLE)
    list(APPEND GLFW_SOURCES
        ${GLFW_DIR}/src/cocoa_init.m
        ${GLFW_DIR}/src/cocoa_joystick.m
        ${GLFW_DIR}/src/cocoa_monitor.m
        ${GLFW_DIR}/src/cocoa_window.m
        ${GLFW_DIR}/src/cocoa_time.c
        ${GLFW_DIR}/src/nsgl_context.m
        ${GLFW_DIR}/src/posix_module.c
        ${GLFW_DIR}/src/posix_thread.c
    )
    add_definitions(-D_GLFW_COCOA -DGL_SILENCE_DEPRECATION)
else()
    list(APPEND GLFW_SOURCES
        ${GLFW_DIR}/src/x11_init.c
        ${GLFW_DIR}/src/x11_monitor.c
        ${GLFW_DIR}/src/x11_window.c
        ${GLFW_DIR}/src/xkb_unicode.c
        ${GLFW_DIR}/src/glx_context.c
        ${GLFW_DIR}/src/linux_joystick.c
        ${GLFW_DIR}/src/posix_module.c
        ${GLFW_DIR}/src/posix_poll.c
        ${GLFW_DIR}/src/posix_thread.c
        ${GLFW_DIR}/src/posix_time.c
    )
    add_definitions(-D_GLFW_X11)
endif()

# ── ImGui ─────────────────────────────────────────────────────────────────────
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# ── imGuIZMO ──────────────────────────────────────────────────────────────────
set(IMGUIZMO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/imGuIZMO)
set(IMGUIZMO_SOURCES
    ${IMGUIZMO_DIR}/imGuIZMOquat.cpp
)

# ── GLAD ──────────────────────────────────────────────────────────────────────
set(GLAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/glad)
set(GLAD_SOURCES ${GLAD_DIR}/src/glad.c)

# ── Executable ────────────────────────────────────────────────────────────────
add_executable(OpenGLApp
    main.cpp
    ${GLFW_SOURCES}
    ${IMGUI_SOURCES}
    ${IMGUIZMO_SOURCES}
    ${GLAD_SOURCES}
)

# ── GLM (header-only) ─────────────────────────────────────────────────────────
set(GLM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/glm)

# ── OpenCL ────────────────────────────────────────────────────────────────────
set(OPENCL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/openCL)

if(APPLE)
    find_library(OpenCL_LIBRARY OpenCL REQUIRED)
    add_library(_OpenCLCompat INTERFACE)
    target_link_libraries(_OpenCLCompat INTERFACE ${OpenCL_LIBRARY})
    target_include_directories(_OpenCLCompat INTERFACE ${OPENCL_DIR})
    add_library(OpenCL::OpenCL ALIAS _OpenCLCompat)
else()
    include(FetchContent)

    set(OPENCL_HEADERS_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        OpenCL-Headers
        GIT_REPOSITORY https://github.com/KhronosGroup/OpenCL-Headers.git
        GIT_TAG        v2024.10.24
    )
    FetchContent_MakeAvailable(OpenCL-Headers)

    set(OPENCL_ICD_LOADER_BUILD_TESTING     OFF CACHE BOOL "" FORCE)
    set(OPENCL_ICD_LOADER_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        OpenCL-ICD-Loader
        GIT_REPOSITORY https://github.com/KhronosGroup/OpenCL-ICD-Loader.git
        GIT_TAG        v2024.10.24
    )
    FetchContent_MakeAvailable(OpenCL-ICD-Loader)
endif()

# ── Include directories ───────────────────────────────────────────────────────
target_include_directories(OpenGLApp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GLFW_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMGUIZMO_DIR}
    ${GLM_DIR}
    ${GLAD_DIR}/include
)

# ── Platform-specific compile options ─────────────────────────────────────────
if(WIN32)
    if(MSVC)
        target_compile_options(OpenGLApp PRIVATE /wd4005)
    endif()
elseif(APPLE)
    target_compile_definitions(OpenGLApp PRIVATE
        CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE=0x10000000)
endif()

# ── Link libraries ────────────────────────────────────────────────────────────
if(WIN32)
    target_link_libraries(OpenGLApp PRIVATE
        ${OPENGL_gl_LIBRARY}
        OpenCL::OpenCL
        gdi32
    )
elseif(APPLE)
    target_link_libraries(OpenGLApp PRIVATE
        ${OPENGL_gl_LIBRARY}
        OpenCL::OpenCL
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreFoundation"
        "-framework OpenGL"
    )
else()
    target_link_libraries(OpenGLApp PRIVATE
        ${OPENGL_gl_LIBRARY}
        OpenCL::OpenCL
        X11
        pthread
        dl
    )
endif()
