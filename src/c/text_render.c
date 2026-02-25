// @file text_render.h
// @brief Text rendering library to draw LECO font
//
// Library to draw LECO font at any size as well as to calculate the
// size of any string at a certain font size. Also allows to draw
// some string at the largest size that will fit inside a certain rectangle.
// All text in this library have half-size kerning on both sides of letters.
//
// @author Eric D. Phillips
// @bug No known bugs

#include "text_render.h"
#include "utility.h"
#include <pebble.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Font Data
//

// general dimensions
static const int8_t LECO_MAX_POINTS = 14; //< Maximum number of points in a single character
static const int8_t LECO_HEIGHT = 20;     //< Font size of raw character data
static const int8_t LECO_KERNING = 4;     //< Spacing between characters
// unscaled centered point data for all LECO font characters
static const int8_t LECO_POINTS_X[] = {
    -07, +07, +07, -07, -07, -03, -03, +03, +03, -03,                     // 0
    -06, +02, +02, +06, +06, -06, -06, -02, -02, -06,                     // 1
    -07, +07, +07, -03, -03, +07, +07, -07, -07, +03, +03, -03, -03, -07, // 2
    -07, +07, +07, -07, -07, +03, +03, -05, -05, +03, +03, -07,           // 3
    -07, -03, -03, +03, +03, +07, +07, +03, +03, -07,                     // 4
    -07, +07, +07, -03, -03, +07, +07, -07, -07, -03, -03, +03, +03, -07, // 5
    +07, -03, -03, +03, +03, -05, -05, +07, +07, -07, -07, +07,           // 6
    -07, +07, +07, +03, +03, -03, -03, -07,                               // 7
    -07, -07, +07, +07, -07, -07, +05, +05, -03, -03, +03, +03, -03, -03, // 8
    -07, +03, +03, -03, -03, +05, +05, -07, -07, +07, +07, -07,           // 9
    -02, +02, +02, -02, -02, +02, +02, -02,                               // :
};
static const int8_t LECO_POINTS_Y[] = {
    -10, -10, +10, +10, -10, -10, +06, +06, -06, -06,                     // 0
    -10, -10, +06, +06, +10, +10, +06, +06, -06, -06,                     // 1
    -10, -10, +02, +02, +06, +06, +10, +10, -02, -02, -06, -06, -04, -04, // 2
    -10, -10, +10, +10, +06, +06, +02, +02, -02, -02, -06, -06,           // 3
    -10, -10, -02, -02, -10, -10, +10, +10, +02, +02,                     // 4
    -10, -10, -06, -06, -02, -02, +10, +10, +04, +04, +06, +06, +02, +02, // 5
    -06, -06, +06, +06, +02, +02, -02, -02, +10, +10, -10, -10,           // 6
    -10, -10, +10, +10, -06, -06, -04, -04,                               // 7
    +02, -10, -10, +10, +10, -02, -02, +02, +02, +06, +06, -06, -06, +02, // 8
    +06, +06, -06, -06, -02, -02, +02, +02, -10, -10, +10, +10,           // 9
    -06, -06, -02, -02, +06, +06, +10, +10,                               // :
};
// offsets into point data for the start of every character padded with the count
static const uint8_t LECO_OFFSETS[] = {0, 10, 20, 34, 46, 56, 70, 82, 90, 104, 116, 124};
// width of each character
static const int8_t LECO_WIDTHS[] = {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 4};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Obtain the character data index from the character value
static uint8_t prv_get_character_index(char character) {
  ASSERT(character >= 48 && character <= 58 && "Invalid character");
  return character - 48;
}

// Draw a LECO character
static void prv_draw_character(GContext *ctx, GPath *path, char character, int16_t font_size) {
  uint8_t char_idx = prv_get_character_index(character);
  uint8_t start_offset = LECO_OFFSETS[char_idx];
  uint8_t end_offset = LECO_OFFSETS[char_idx + 1];
  // populate the path data points
  path->num_points = end_offset - start_offset;
  for (uint8_t ii = 0, idx = start_offset; idx < end_offset; ii++, idx++) {
    path->points[ii].x = LECO_POINTS_X[idx] * font_size / LECO_HEIGHT;
    path->points[ii].y = LECO_POINTS_Y[idx] * font_size / LECO_HEIGHT;
  }
  // calculate the next cursor position
  int16_t next_x =
      path->offset.x + (LECO_WIDTHS[char_idx] + LECO_KERNING) * font_size / LECO_HEIGHT;
  // offset to half the character and draw
  path->offset.x = (path->offset.x + next_x) / 2;
  gpath_draw_filled(ctx, path);
  gpath_draw_outline(ctx, path);
  // restore the next cursor position
  path->offset.x = next_x;
}

// Draw the LECO font onto a drawing context at a certain font size
static void prv_draw_text(GContext *ctx, char *buff, int16_t font_size, GPoint position) {
  // offset the cursor with half kerning size (for legacy reasons)
  position.x += (LECO_KERNING * font_size / LECO_HEIGHT) / 2;
  // initialize the reusable graphics path
  GPoint point_data[LECO_MAX_POINTS];
  GPath path = (GPath){
      .points = point_data,
      .offset = position,
  };
  // loop over characters in buff until NUL character is reached
  for (uint8_t ii = 0; buff[ii] != '\0'; ii++) {
    prv_draw_character(ctx, &path, buff[ii], font_size);
  }
}

static int16_t prv_unscaled_text_width(char *buff) {
  int16_t total_width = 0;
  for (uint8_t ii = 0; buff[ii] != '\0'; ii++) {
    uint8_t char_idx = prv_get_character_index(buff[ii]);
    total_width += LECO_WIDTHS[char_idx] + LECO_KERNING; // Always add kerning for legacy reasons
  }
  return total_width;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// API Functions
//

// Gets the bounds of a certain text string at a certain font size
GRect text_render_get_content_bounds(char *buff, int16_t font_size) {
  if (buff == NULL || buff[0] == '\n' || font_size == 0) {
    return GRectZero;
  }
  int16_t total_width = prv_unscaled_text_width(buff);
  return GRect(0, 0, total_width * font_size / LECO_HEIGHT, font_size);
}

// Gets the maximum font size of a certain text string within a certain bounds
int16_t text_render_get_max_font_size(char *buff, GRect bounds) {
  if (buff == NULL || buff[0] == '\n' || bounds.size.w <= 0 || bounds.size.h <= 0) {
    return 0;
  }
  // calculate the maximum font size which stays within this rectangle
  int16_t total_width = prv_unscaled_text_width(buff);
  int16_t font_size_w = LECO_HEIGHT * bounds.size.w / total_width;
  int16_t font_size_h = bounds.size.h;
  return (font_size_h < font_size_w) ? font_size_h : font_size_w;
}

// Renders the LECO font onto a drawing context at a certain font size
void text_render_draw_text(GContext *ctx, char *buff, int16_t font_size, GPoint position) {
  if (buff == NULL || buff[0] == '\n' || font_size == 0) {
    return;
  }
  prv_draw_text(ctx, buff, font_size, position);
}

// Renders the LECO font at the largest possible size that will fit within a certain size rectangle
void text_render_draw_scalable_text(GContext *ctx, char *buff, GRect bounds) {
  if (buff == NULL || buff[0] == '\n' || bounds.size.w <= 0 || bounds.size.h <= 0) {
    return;
  }
  // calculate the maximum font size which stays within this rectangle
  int16_t total_width = prv_unscaled_text_width(buff);
  int16_t font_size_w = LECO_HEIGHT * bounds.size.w / total_width;
  int16_t font_size_h = bounds.size.h;
  int16_t font_size = (font_size_h < font_size_w) ? font_size_h : font_size_w;
  // center the text
  GPoint position;
  position.x = bounds.origin.x + (bounds.size.w - total_width * font_size / LECO_HEIGHT) / 2;
  position.y = bounds.origin.y + bounds.size.h / 2;
  // draw the text onto the graphics context
  prv_draw_text(ctx, buff, font_size, position);
}
