// @file interpolation.c
// @brief Various interpolation for animations
//
// File contains different interpolation functions including a number
// of different easing modes. Several convenience functions also exist
// for interpolating different Pebble data types
//
// @author Eric D. Phillips
// @date October 31, 2015
// @bug No known bugs

#include <pebble.h>
#include "interpolation.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Linear interpolation
static int32_t prv_curve_linear(int32_t from, int32_t to, uint32_t percent, uint32_t percent_max) {
  return from + (to - from) * (int32_t)percent / (int32_t)percent_max;
}

// Quadratic interpolation in
static int32_t prv_curve_quad_ease_in(int32_t from, int32_t to, uint32_t percent,
                                      uint32_t percent_max) {
  int32_t t_percent = percent * 100 / percent_max;
  return (to - from) * t_percent * t_percent / 10000 + from;
}

// Quadratic interpolation out
static int32_t prv_curve_quad_ease_out(int32_t from, int32_t to, uint32_t percent,
                                       uint32_t percent_max) {
  int32_t t_percent = percent * 100 / percent_max;
  return - (to - from) * t_percent * (t_percent - 200) / 10000 + from;
}

// Quadratic interpolation in out
static int32_t prv_curve_quad_ease_in_out(int32_t from, int32_t to, uint32_t percent,
                                          uint32_t percent_max) {
  int32_t t_percent = percent * 100 / (percent_max / 2);
  if (t_percent < 100) {
    return (to - from) / 2 * t_percent * t_percent / 10000 + from;
  }
  t_percent -= 100;
  return -(to - from) / 2 * (t_percent * (t_percent - 200) - 10000) / 10000 + from;
}

// Sinusoidal interpolation in
static int32_t prv_curve_sin_ease_in(int32_t from, int32_t to, uint32_t percent,
                                     uint32_t percent_max) {
  return (-((to - from) / 2) * cos_lookup(TRIG_MAX_ANGLE * percent / percent_max / 4)) /
  (TRIG_MAX_RATIO / 2) + (to - from) + from;
}

// Sinusoidal interpolation out
static int32_t prv_curve_sin_ease_out(int32_t from, int32_t to, uint32_t percent,
                                      uint32_t percent_max) {
  return (((to - from) / 2) * sin_lookup(TRIG_MAX_ANGLE * percent / percent_max / 4)) /
    (TRIG_MAX_RATIO / 2) + from;
}

// Sinusoidal interpolation in out
static int32_t prv_curve_sin_ease_in_out(int32_t from, int32_t to, uint32_t percent,
                                         uint32_t percent_max) {
  return (-((to - from) / 2) * (cos_lookup(TRIG_MAX_ANGLE * percent / percent_max / 2) -
    (TRIG_MAX_RATIO / 2)) / 2) / TRIG_MAX_RATIO + from;
}

// Array of interpolation function pointers
typedef int32_t (*InterpolationFunction)(int32_t, int32_t, uint32_t, uint32_t);
InterpolationFunction interpolation_functions[] = {
  prv_curve_linear,
  prv_curve_quad_ease_in,
  prv_curve_quad_ease_out,
  prv_curve_quad_ease_in_out,
  prv_curve_sin_ease_in,
  prv_curve_sin_ease_out,
  prv_curve_sin_ease_in_out
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Functions
//

// Interpolate an integer
int32_t interpolation_integer(int32_t from, int32_t to, uint32_t percent, uint32_t percent_max,
                              InterpolationCurve curve) {
  if (percent >= percent_max) {
    return to;
  }
  return interpolation_functions[curve](from, to, percent, percent_max);
}

// Interpolate a GPoint
GPoint interpolation_gpoint(GPoint from, GPoint to, uint32_t percent, uint32_t percent_max,
                            InterpolationCurve curve) {
  if (percent >= percent_max) {
    return to;
  }
  return GPoint(interpolation_integer(from.x, to.x, percent, percent_max, curve),
                interpolation_integer(from.y, to.y, percent, percent_max, curve));
}
