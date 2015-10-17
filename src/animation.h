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

#include <pebble.h>

//! Animate an integer by its pointer
//! @param ptr A pointer to the integer to animate
//! @param to The value to animate the pointer value to
//! @param duration The length of time over which to animate the value
void animation_uint32_start(uint32_t *ptr, uint32_t to, uint32_t duration);

//! Animate a GRect by its pointer
//! @param ptr A pointer to the GRect to animate
//! @param to The GRect to animate the pointer value to
//! @param duration The length of time over which to animate the value
void animation_grect_start(GRect *ptr, GRect to, uint32_t duration);

//! Cancel an animation by its pointer
//! @param ptr A pointer for which to cancel an animation
void animation_stop(void *ptr);
