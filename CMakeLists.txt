cmake_minimum_required(VERSION 3.20)
project(TestNUKEModule CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Match the rest of the toolchain: dynamic debug CRT (/MDd in Debug).
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

set(NUKE_ROOT       "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(VCPKG_INSTALLED "$ENV{VCPKG_ROOT}/installed/x64-windows")   # VCPKG_ROOT env var, not a hardcoded path

add_library(TestNUKEModule SHARED
    testnukeemodule.cpp
)

# NOTE: do NOT define NUKEENGINE_EXPORTS — as a consumer the plugin sees the engine's
# NUKEENGINE_API symbols as dllimport and links NukeEngine.lib (the import lib).
# IMGUI_API=dllimport: ImGui comes from the shared NukeImGui.dll (one context for all).
target_compile_definitions(TestNUKEModule PRIVATE
    WIN32 _WINDOWS NOMINMAX _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS
    GLM_ENABLE_EXPERIMENTAL
    BOOST_ALL_DYN_LINK
    IMGUI_API=__declspec\(dllimport\)   # escape parens so the VS generator keeps it as one define
)

target_include_directories(TestNUKEModule PRIVATE
    ${NUKE_ROOT}/NukeEngine
    ${NUKE_ROOT}/NukeEngine/include
    ${NUKE_ROOT}/NukeEngine/deps
    ${NUKE_ROOT}/NukeEngine/deps/glm
    ${NUKE_ROOT}/NukeEngine/deps/LuaBridge/Source
    ${NUKE_ROOT}/NukeImGui          # so <imgui/imgui.h> resolves
    ${NUKE_ROOT}/NukeImGui/imgui     # so imgui's own internal includes resolve
    ${VCPKG_INSTALLED}/include
)

target_link_directories(TestNUKEModule PRIVATE
    ${NUKE_ROOT}/NukeEngine/x64/$<CONFIG>   # NukeEngine.lib (import lib)
    ${NUKE_ROOT}/NukeImGui/build/$<CONFIG>  # NukeImGui.lib
    ${VCPKG_INSTALLED}/$<$<CONFIG:Debug>:debug/>lib        # boost (auto-linked)
)

target_link_libraries(TestNUKEModule PRIVATE
    NukeEngine
    NukeImGui
)

# Deploy the plugin DLL into the editor's modules/ folder.
set(MODULES_DIR "${NUKE_ROOT}/NukeEngine/x64/$<CONFIG>/modules")
add_custom_command(TARGET TestNUKEModule POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MODULES_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:TestNUKEModule>" "${MODULES_DIR}/"
    COMMENT "Deploying TestNUKEModule.dll to modules/"
)
