//! @file animation.h
//! @brief Animation framework to animate pointer values
//!
//! Animation framework to animate a pointer's value. Includes automatic
//! detection of multiple animations per pointer, and destroys the oldest one.
//! Animations also auto-destruct when complete
//!
//! @author Eric D. Phillips
//! @date September 1, 2015
//! @bugs No known bugs

#pragma once
#include <pebble.h>
#include "interpolation.h"

//! Animate a GRect by its pointer
//! @param prt A pointer to the GRect to animate
//! @param to The GRect to animate the pointer to
//! @param duration The length of time over which to animate the GRect
//! @param delay The length of time to wait before running the animation
//! @param interpolation The interpolation mode to use for the animation
void animation_grect_start(GRect *ptr, GRect to, uint32_t duration, uint32_t delay,
                           InterpolationCurve interpolation);

//! Animate an integer by its pointer
//! @param ptr A pointer to the integer to animate
//! @param to The value to animate the pointer value to
//! @param duration The length of time over which to animate the value
//! @param delay The length of time to wait before running the animation
//! @param interpolation The interpolation mode to use for the animation
void animation_int32_start(int32_t *ptr, int32_t to, uint32_t duration, uint32_t delay,
                           InterpolationCurve interpolation);

//! Cancel an animation by its pointer
//! @param ptr A pointer for which to cancel an animation
void animation_stop(void *ptr);

//! Cancel all running animations
void animation_stop_all(void);

//! Register animation update callback
//! @param callback A pointer to the function to call when updating
void animation_register_update_callback(void *callback);
