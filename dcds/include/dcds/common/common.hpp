//
// Created by Aunn Raza on 19.04.23.
//

#ifndef DCDS_COMMON_HPP
#define DCDS_COMMON_HPP

#include <absl/log/log.h>

#include "dcds/common/types.hpp"

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

#endif  // DCDS_COMMON_HPP
