/**
 * @file tapioca.cpp
 * @brief Tapioca library entry point — version info.
 */

#include "tapioca/exports.h"

namespace tapioca {

TAPIOCA_API const char *version() noexcept {
    return "0.1.0";
}

} // namespace tapioca
