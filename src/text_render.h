//! @file text_render.h
//! @brief Text rendering library to draw LECO font
//!
//! Library to draw LECO font at any size as well as to calculate the
//! size of any string at a certain font size. Also allows to draw
//! some string at the largest size that will fit inside a certain rectangle.
//! All text in this library have half-size kerning on both sides of letters.
//!
//! @author Eric D. Phillips
//! @bug No known bugs

#pragma once
#include <pebble.h>

//! Gets the size of a certain text string at a certain font size
//! @param buff The text string to measure
//! @param font_size The font size to measure the string at
GSize text_render_get_content_size(char *buff, uint16_t font_size);

//! Gets the maximum font size of a certain text string within a certain bounds
//! @param buff The text string to measure
//! @param size The size of the bounds within which to measure the maximum size
uint16_t text_render_get_max_font_size(char *buff, GSize size);

//! Renders the LECO font onto a drawing context at a certain font size
//! @param ctx The GContext onto which to draw the text
//! @param buff The text buffer to draw
//! @param font_size The font size at which to draw the text
//! @param position The upper right corner of where to begin drawing from
void text_render_draw_text(GContext *ctx, char *buff, uint16_t font_size, GPoint position);

//! Renders the LECO font at the largest possible size that will fit within a certain size rectangle
//! @param ctx The GContext onto which to draw the text
//! @param buff The text buffer to draw
//! @param bounds The bounds within which to draw the text at maximum size
void text_render_draw_scalable_text(GContext *ctx, char *buff, GRect bounds);
