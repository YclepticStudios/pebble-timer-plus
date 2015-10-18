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

#include <pebble.h>

// Constants
static const uint16_t CHARACTER_DEFINITION_HEIGHT = 255;
static const uint16_t CHARACTER_DEFINITION_KERNING = 50;
static const uint8_t STRING_MAX_LENGTH = 64;

// Structure for the data contained in one font character
typedef struct {
  char      character;    //< The character being represented
  uint8_t   char_width;   //< The width of the character (compare to
  uint8_t   num_points;   //< The number of points in that character
  GPoint    points[14];   //< The array of points for that character
} Character;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Font Data
//

// This is the data for the LECO font. Each character is composed of an array of points,
// that, when drawn filled, will form the character
static Character LECO_0 = {'0', 178, 10, {{0, 0}, {178, 0}, {178, 255}, {0, 255}, {0, 0},
                                          {50, 0}, {50, 205}, {128, 205}, {128, 50}, {0, 50}}};
static Character LECO_1 = {'1', 178, 10, {{0, 0}, {114, 0}, {114, 205}, {178, 205}, {178, 255},
                                          {0, 255}, {0, 205}, {64, 205}, {64, 50}, {0, 50}}};
static Character LECO_2 = {'2', 178, 14, {{0, 68}, {0, 0}, {178, 0}, {178, 153}, {50, 153},
                                          {50, 205}, {178, 205}, {178, 255}, {0, 255}, {0, 103},
                                          {128, 103}, {128, 50}, {50, 50}, {50, 68}}};
static Character LECO_3 = {'3', 178, 12, {{0, 0}, {178, 0}, {178, 255}, {0, 255}, {0, 205},
                                          {128, 205}, {128, 153}, {26, 153}, {26, 103}, {128, 103},
                                          {128, 50}, {0, 50}}};
static Character LECO_4 = {'4', 178, 10, {{0, 0}, {0, 153}, {128, 153}, {128, 255}, {178, 255},
                                          {178, 0}, {128, 0}, {128, 103}, {50, 103}, {50, 0}}};
static Character LECO_5 = {'5', 178, 14, {{178, 0},{0, 0}, {0, 153}, {128, 153}, {128, 205},
                                          {50, 205}, {50, 187}, {0, 187}, {0, 255}, {178, 255},
                                          {178, 103}, {50, 103}, {50, 50}, {178, 50}}};
static Character LECO_6 = {'6', 178, 12, {{178, 0}, {0, 0}, {0, 255}, {178, 255}, {178, 103},
                                          {25, 103}, {25, 153}, {128, 153}, {128, 205}, {50, 205},
                                          {50, 50}, {178, 50}}};
static Character LECO_7 = {'7', 178, 8, {{0, 76}, {0, 0}, {178, 0}, {178, 255}, {128, 255},
                                         {128, 50}, {50, 50}, {50, 76}}};
static Character LECO_8 = {'8', 178, 14, {{0, 153}, {0, 0}, {178, 0}, {178, 255}, {0, 255},
                                          {0, 103}, {163, 103}, {163, 153}, {50, 153}, {50, 205},
                                          {128, 205}, {128, 50}, {50, 50}, {50, 153}}};
static Character LECO_9 = {'9', 178, 12, {{0, 255}, {178, 255}, {178, 0}, {0, 0}, {0, 153},
                                          {163, 153}, {163, 103}, {50, 103}, {50, 50}, {128, 50},
                                          {128, 205}, {0, 205}}};
static Character LECO_C = {':', 50, 4, {{0, 50}, {50, 50}, {50, 100}, {0, 100}}};
static Character LECO_P = {'.', 50, 4, {{0, 205}, {50, 205}, {50, 255}, {0, 255}}};

static Character *LECO_CHARS[] = {&LECO_0, &LECO_1, &LECO_2, &LECO_3, &LECO_4, &LECO_5,
                                  &LECO_6, &LECO_7, &LECO_8, &LECO_9, &LECO_C, &LECO_P};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Draw a LECO character
static void prv_draw_character(GContext *ctx, GPath *path, Character *leco_char, uint16_t font_size,
                               GPoint position) {
  // set the path params
  path->num_points = leco_char->num_points;
  memcpy(path->points, leco_char->points, sizeof(leco_char->points));
  for (uint8_t kk = 0; kk < leco_char->num_points; kk++) {
    path->points[kk].x = path->points[kk].x * font_size / CHARACTER_DEFINITION_HEIGHT;
    path->points[kk].y = path->points[kk].y * font_size / CHARACTER_DEFINITION_HEIGHT;
  }
  path->offset = position;
  gpath_draw_filled(ctx, path);
  gpath_draw_outline(ctx, path);
}

// Draw the LECO font onto a drawing context at a certain font size
static void prv_draw_text(GContext *ctx, char *buff, uint16_t font_size, GPoint position) {
  // graphics path to draw
  GPoint cur_origin = position;
  cur_origin.x += (CHARACTER_DEFINITION_KERNING * font_size / CHARACTER_DEFINITION_HEIGHT) / 2;
  GPath path = (GPath) {
    .points = (GPoint*)malloc(sizeof(LECO_CHARS[0]->points)),
    .offset = cur_origin,
  };
  // loop over characters in buff until NUL character is reached
  for (uint8_t ii = 0; ii < STRING_MAX_LENGTH && buff[ii] != '\0'; ii++) {
    // find that character in the array of data
    for (uint8_t jj = 0; jj < ARRAY_LENGTH(LECO_CHARS); jj++) {
      if (LECO_CHARS[jj]->character == buff[ii]) {
        // draw the character
        prv_draw_character(ctx, &path, LECO_CHARS[jj], font_size, cur_origin);
        // draw an extra spot for the colon (colon only contains data for top dot)
        if (LECO_CHARS[jj]->character == ':') {
          prv_draw_character(ctx, &path, &LECO_P, font_size, cur_origin);
        }
        // recalculate current character origin (top left)
        cur_origin.x += (LECO_CHARS[jj]->char_width + CHARACTER_DEFINITION_KERNING) *
                        font_size / CHARACTER_DEFINITION_HEIGHT;
        break;
      }
    }
  }
  // free memory
  free(path.points);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Functions
//

// Gets the size of a certain text string at a certain font size
GSize text_render_get_content_size(char *buff, uint16_t font_size) {
  // get the unscaled size of the rendered string
  int16_t total_width = 0;
  for (uint8_t ii = 0; ii < STRING_MAX_LENGTH && buff[ii] != '\0'; ii++) {
    // find that character in the array of data
    for (uint8_t jj = 0; jj < ARRAY_LENGTH(LECO_CHARS); jj++) {
      if (LECO_CHARS[jj]->character == buff[ii]) {
        // add the width of that character to the total width
        total_width += LECO_CHARS[jj]->char_width + CHARACTER_DEFINITION_KERNING;
        break;
      }
    }
  }

  // return the size
  return GSize(total_width * font_size / CHARACTER_DEFINITION_HEIGHT, font_size);
}

// Gets the maximum font size of a certain text string within a certain bounds
uint16_t text_render_get_max_font_size(char *buff, GSize size) {
  // get the unscaled size of the rendered string
  GSize unscaled_size = text_render_get_content_size(buff, CHARACTER_DEFINITION_HEIGHT);
  // calculate the maximum font size which stays within this rectangle
  uint16_t font_size_w = CHARACTER_DEFINITION_HEIGHT * size.w / unscaled_size.w;
  uint16_t font_size_h = CHARACTER_DEFINITION_HEIGHT * size.h / unscaled_size.h;
  return (font_size_h < font_size_w) ? font_size_h : font_size_w;
}

// Renders the LECO font onto a drawing context at a certain font size
void text_render_draw_text(GContext *ctx, char *buff, uint16_t font_size, GPoint position) {
  // draw the text at a certain font size
  prv_draw_text(ctx, buff, font_size, position);
}

// Renders the LECO font at the largest possible size that will fit within a certain size rectangle
void text_render_draw_scalable_text(GContext *ctx, char *buff, GRect bounds) {
  // get the unscaled size of the rendered string
  GSize unscaled_size = text_render_get_content_size(buff, CHARACTER_DEFINITION_HEIGHT);
  // calculate the maximum font size which stays within this rectangle
  uint16_t font_size_w = CHARACTER_DEFINITION_HEIGHT * bounds.size.w / unscaled_size.w;
  uint16_t font_size_h = CHARACTER_DEFINITION_HEIGHT * bounds.size.h / unscaled_size.h;
  uint16_t font_size = (font_size_h < font_size_w) ? font_size_h : font_size_w;
  // center the text
  GPoint position;
  position.x = bounds.origin.x + (bounds.size.w - unscaled_size.w * font_size /
                                                  CHARACTER_DEFINITION_HEIGHT) / 2;
  position.y = bounds.origin.y + (bounds.size.h - unscaled_size.h * font_size /
                                                  CHARACTER_DEFINITION_HEIGHT) / 2;
  // draw the text onto the graphics context
  prv_draw_text(ctx, buff, font_size, position);
}
