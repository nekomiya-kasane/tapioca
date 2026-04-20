# tapioca — Terminal rendering infrastructure (COCA workspace build)
# Provides: tapioca (SHARED), tapioca::tapioca
# Layer 1: depends on icetea

# ── Sources ──────────────────────────────────────────────────────────────
set(TAPIOCA_SOURCES
    src/tapioca.cpp
    src/style.cpp
    src/unicode_width.cpp
    src/pal.cpp
    src/image.cpp
    src/console.cpp
)

if(EMSCRIPTEN)
    list(APPEND TAPIOCA_SOURCES
        src/terminal_emscripten.cpp
        src/raw_input_emscripten.cpp
    )
else()
    list(APPEND TAPIOCA_SOURCES
        src/terminal.cpp
        src/raw_input.cpp
    )
endif()

# ── Shared library ───────────────────────────────────────────────────────
if(EMSCRIPTEN)
    add_library(tapioca STATIC ${TAPIOCA_SOURCES})
else()
    add_library(tapioca SHARED ${TAPIOCA_SOURCES})
endif()
add_library(tapioca::tapioca ALIAS tapioca)

target_include_directories(tapioca
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(tapioca PRIVATE TAPIOCA_BUILD_INTERNAL)

target_link_libraries(tapioca PUBLIC icetea::icetea)

if(MSVC)
    target_compile_definitions(tapioca PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(tapioca PRIVATE /W4 /utf-8 /permissive-)
    target_compile_options(tapioca PRIVATE
        $<$<CONFIG:Release>:/Oi /GS- /fp:fast /GL>
        $<$<CONFIG:RelWithDebInfo>:/Oi /fp:fast>
    )
    target_link_options(tapioca PRIVATE
        $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
    )
else()
    target_compile_options(tapioca PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ── Tests ────────────────────────────────────────────────────────────────
if(TARGET gtest AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()
