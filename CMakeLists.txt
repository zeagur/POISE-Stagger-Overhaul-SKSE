# It's recommended to set a minimum CMake version.
# If you use CMake features from higher versions, update this to match.
cmake_minimum_required(VERSION 3.21)

# Set your project name. This will be the name of your SKSE .dll file.
project(
        loki_POISE
        VERSION 1.0.0
        LANGUAGES CXX
)

# ---- Include guards ----
if (PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(
            FATAL_ERROR
            "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
    )
endif ()

# Setup your SKSE plugin as an SKSE plugin!
add_definitions(-DUNICODE -D_UNICODE)
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
file(
        GLOB
        SOURCE_FILES
        "src/*.cpp"
        "src/*.h"
        "src/POISE/*.cpp"
        "src/POISE/*.h"
)

include(GNUInstallDirs)
include(FetchContent)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

set(SPDLOG_INSTALL ON)
set(SKSE_SUPPORT_XBYAK ON)

# in-place configuration
source_group(
        TREE
        ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        ${SOURCE_FILES}
)

# ---- Dependencies ----
#find_package(boost-algorithm CONFIG REQUIRED)
#find_package(rapidxml CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

FetchContent_Declare(
        CommonLibSSE
        GIT_REPOSITORY https://github.com/CharmedBaryon/CommonLibSSE-NG.git
        GIT_TAG main
        OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(CommonLibSSE)
include(${CommonLibSSE_SOURCE_DIR}/cmake/CommonLibSSE.cmake) # for the add_commonlibsse_plugin to work

# ---- Create DLL ----
add_commonlibsse_plugin(
        ${PROJECT_NAME}
        SOURCES
        ${headers}
        ${SOURCE_FILES}
        .clang-format
)

target_compile_features(
        ${PROJECT_NAME}
        PRIVATE
        cxx_std_23
)

target_compile_definitions(
        ${PROJECT_NAME}
        PRIVATE
        _UNICODE
)

target_include_directories(
        ${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${SIMPLEINI_INCLUDE_DIRS}
)

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        CommonLibSSE::CommonLibSSE
)

target_precompile_headers(
        ${PROJECT_NAME}
        PRIVATE
        src/PCH.h
)

target_compile_options(
        ${PROJECT_NAME}
        PRIVATE
        /W0
        /utf-8
        /MP
        /await
        /permissive-
        /Zc:alignedNew
        /Zc:auto
        /Zc:__cplusplus
        /Zc:externC
        /Zc:externConstexpr
        /Zc:forScope
        /Zc:hiddenFriend
        /Zc:implicitNoexcept
        /Zc:lambda
        /Zc:noexceptTypes
        /Zc:preprocessor
        /Zc:referenceBinding
        /Zc:rvalueCast
        /Zc:sizedDealloc
        /Zc:strictStrings
        /Zc:ternary
        /Zc:threadSafeInit
        /Zc:trigraphs
        /Zc:wchar_t
        /wd4200 # nonstandard extension used : zero-sized array in struct/union
)