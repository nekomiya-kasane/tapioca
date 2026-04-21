#pragma once

/// @file exports.h
/// @brief DLL export/import macros and compiler optimization hints for tapioca.

#if defined(TAPIOCA_BUILD_INTERNAL)
#    if defined(_MSC_VER)
#        define TAPIOCA_API __declspec(dllexport)
#    elif defined(__GNUC__) || defined(__clang__)
#        define TAPIOCA_API __attribute__((visibility("default")))
#    else
#        define TAPIOCA_API
#    endif
#else
#    if defined(_MSC_VER)
#        define TAPIOCA_API __declspec(dllimport)
#    else
#        define TAPIOCA_API
#    endif
#endif

// ── Optimization hints ────────────────────────────────────────────────────
#if defined(_MSC_VER)
#    define TAPIOCA_NOINLINE __declspec(noinline)
#    define TAPIOCA_COLD __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#    define TAPIOCA_NOINLINE __attribute__((noinline))
#    define TAPIOCA_COLD __attribute__((noinline, cold))
#else
#    define TAPIOCA_NOINLINE
#    define TAPIOCA_COLD
#endif
