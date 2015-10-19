//! @file drawing.h
//! @brief Main drawing code
//!
//! Contains all the drawing code for this app.
//!
//! @author Eric D. Phillips
//! @date August 29, 2015
//! @bugs No known bugs

#pragma once
#include <pebble.h>

//! Update the progress ring angle based on the timer values
void drawing_update_progress_ring_angle(void);

//! Render everything to the screen
//! @param layer The layer being rendered onto
//! @param ctx The layer's drawing context
void drawing_render(Layer *layer, GContext *ctx);

//! Initialize the singleton drawing data
//! @param layer The layer which the drawing code can force to refresh, for animations
void drawing_initialize(Layer *layer);

//! Destroy the singleton drawing data
void drawing_terminate(void);
