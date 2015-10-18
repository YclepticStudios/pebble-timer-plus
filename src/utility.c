// @file utility.c
// @brief File containing simple convenience functions.
//
// This file contains simple convenience functions that may be used
// in several different places. An example would be the "assert" function
// which terminates program execution based on the state of a pointer.
//
// @author Eric D. Phillips
// @date August 29, 2015
// @bugs No known bugs

#include "utility.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
// Compatibility Functions for Aplite
//

#ifdef PBL_SDK_2
// Get a point from a center point, angle, and radius
static GPoint prv_polar_to_rectangular(GPoint center, int32_t angle, int16_t radius) {
  return GPoint((sin_lookup(angle) * radius / TRIG_MAX_RATIO) + center.x,
                (-cos_lookup(angle) * radius / TRIG_MAX_RATIO) + center.y);
}

// Draw a filled arc
void graphics_fill_radial(GContext *ctx, GRect bounds, uint8_t fill_mode, int16_t inset,
                          int32_t angle_start, int32_t angle_end) {
  // get step angle and exit if too small
  int32_t step = (angle_end - angle_start) / 4;
  if (step < 1) {
    return;
  }
  // get properties
  GPoint center = grect_center_point(&bounds);
  int16_t radius = (bounds.size.w + bounds.size.h) / 2;
  // calculate points around outside of window to draw cover
  GPoint points[8];
  uint32_t idx = 0;
  for (int32_t t_angle = angle_start; t_angle < angle_end; t_angle += step){
    points[idx++] = prv_polar_to_rectangular(center, t_angle, radius);
  }
  // add point at hand position, and in center (to form pie wedge)
  points[idx++] = prv_polar_to_rectangular(center, angle_end, radius);
  points[idx++] = center;

  // fill the covering
  GPathInfo info = (GPathInfo) {
    .num_points = idx,
    .points = points
  };
  GPath *path = gpath_create(&info);
  gpath_draw_filled(ctx, path);
  gpath_destroy(path);
}
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
// Convenience Functions
//

// Check pointer for null and assert if true
void assert(void *ptr, const char *file, int line) {
  if (ptr) {
    return;
  }
  APP_LOG(APP_LOG_LEVEL_ERROR, "Invalid pointer: (%s:%d)", file, line);
  // assert
  void (*exit)(void) = NULL;
  exit();
}

// Malloc with built in pointer check
void *malloc_check(uint16_t size, const char *file, int line) {
  void *ptr = malloc(size);
  assert(ptr, file, line);
  return ptr;
}

// Get current epoch in milliseconds
uint64_t epoch(void) {
  return (uint64_t)time(NULL) * 1000 + (uint64_t)time_ms(NULL, NULL);
}
