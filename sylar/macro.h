#pragma once

#include "util.h"

#include <assert.h>
#include <string>

#define SYLAR_ASSERT(x) \
  if(!(x)) { \
    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: "  #x \
      << "\nBackTrace:\n" \
      << sylar::BacktraceToString(100, 2, "    "); \
    assert(x); \
  }

#define SYLAR_ASSERT2(x, w) \
  if(!(x)) { \
    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x \
      << "\n" << w \
      << "\nBackTrace:\n" \
      << sylar::BacktraceToString(100, 2, "    "); \
    assert(x); \
  }
