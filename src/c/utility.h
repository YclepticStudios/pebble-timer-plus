//! @file utility.h
//! @brief File containing simple convenience functions.
//!
//! This file contains simple convenience functions that may be used
//! in several different places. An example would be the "assert" function
//! which terminates program execution based on the state of a pointer.
//!
//! @author Eric D. Phillips
//! @date August 29, 2015
//! @bugs No known bugs

#pragma once
#include <pebble.h>

//! Time span conversions
#define MSEC_IN_HR 3600000
#define MSEC_IN_MIN 60000
#define MSEC_IN_SEC 1000
#define SEC_IN_MIN 60
#define MIN_IN_HR 60

//! Compatibility functions for Aplite
#ifdef PBL_SDK_2
#define GEdgeInsets1(value) value
GRect grect_inset(GRect bounds, int16_t inset);
static const uint8_t GOvalScaleModeFillCircle = 0;
void graphics_fill_radial(GContext *ctx, GRect bounds, uint8_t fill_mode, int16_t inset,
                          int32_t angle_start, int32_t angle_end);
#endif

#ifdef PBL_BW
//! Fill GRect with "grey" on Aplite
void graphics_fill_rect_grey(GContext *ctx, GRect rect);
#endif

//! Standard assertion definition
#ifdef NDEBUG
#define ASSERT(expression) ((void)0)
#else
#define ASSERT(expression)                                                                         \
  if (!(expression)) {                                                                             \
    APP_LOG(APP_LOG_LEVEL_ERROR, "Assertion failed: %s (%s:%d)", #expression, __FILE__, __LINE__); \
    void (*exit)(void) = NULL;                                                                     \
    exit();                                                                                        \
  }
#endif

//! Malloc with failure check
//! @param size The size of the memory to allocate
#define MALLOC(size) malloc_check(size, __FILE__, __LINE__)

//! Malloc with failure check
//! @param size The size of the memory to allocate
//! @param file The name of the file it is called from
//! @param line The line number it is called from
void *malloc_check(uint16_t size, const char *file, int line);

//! Get current epoch in milliseconds
//! @return The current epoch time in milliseconds
uint64_t epoch(void);
