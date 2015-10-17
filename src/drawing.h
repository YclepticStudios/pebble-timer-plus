//! @file drawing.h
//! @brief Main drawing code
//!
//! Contains all the drawing code for this app.
//!
//! @author Eric D. Phillips
//! @date August 29, 2015
//! @bugs No known bugs

#include <pebble.h>

//! Set the current timer value used when drawing
//! @param current_value The current time on the timer (milliseconds)
void drawing_set_current_value(int64_t current_value);

//! Set the total timer value used when drawing
//! @param total_value The total time on the timer (milliseconds)
void drawing_set_total_value(int64_t total_value);

//! Render everything to the screen
//! @param layer The layer being rendered onto
//! @param ctx The layer's drawing context
void drawing_render(Layer *layer, GContext *ctx);

//! Initialize the singleton drawing data
//! @param layer The layer which the drawing code can force to refresh, for animations
void drawing_initialize(Layer *layer);

//! Destroy the singleton drawing data
void drawing_terminate(void);
