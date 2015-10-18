// @file drawing.h
// @brief Main drawing code
//
// Contains all the drawing code for this app.
//
// @author Eric D. Phillips
// @date August 29, 2015
// @bugs No known bugs

#include <pebble.h>
#include "animation.h"
#include "text_render.h"
#include "utility.h"

// Drawing constants
#define CIRCLE_RADIUS 60
#define ANGLE_CHANGE_ANI_THRESHOLD 348
#define PROGRESS_ANI_DURATION 500
#define MAIN_TEXT_CIRCLE_RADIUS (CIRCLE_RADIUS - 7)
#define MAIN_TEXT_BOUNDS GRect(-MAIN_TEXT_CIRCLE_RADIUS, -MAIN_TEXT_CIRCLE_RADIUS / 2,\
 MAIN_TEXT_CIRCLE_RADIUS * 2, MAIN_TEXT_CIRCLE_RADIUS)

// Main data
static struct {
  Layer       *layer;             //< The main layer being drawn on, used to force a refresh
  int64_t     current_value;      //< The current timer time value (milliseconds)
  int64_t     total_value;        //< The total timer time value (milliseconds)
  int32_t     progress_angle;     //< The current angle of the progress ring
  GColor      fore_color;         //< Color of text
  GColor      mid_color;          //< Color of center
  GColor      ring_color;         //< Color of ring
  GColor      back_color;         //< Color behind ring
} drawing_data;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Data Points for Visuals
//


////////////////////////////////////////////////////////////////////////////////////////////////////
// Progress Ring
//

// Draw progress ring
static void prv_render_progress_ring(GContext *ctx, GRect bounds) {
  // calculate ring bounds size
  int32_t gr_angle = atan2_lookup(bounds.size.h, bounds.size.w);
  int32_t radius = (bounds.size.h / 2) * TRIG_MAX_RATIO / sin_lookup(gr_angle);
  bounds.origin.x += bounds.size.w / 2 - radius;
  bounds.origin.y += bounds.size.h / 2 - radius;
  bounds.size.w = bounds.size.h = radius * 2;
  // draw ring on context
  int32_t angle_1 = drawing_data.progress_angle;
  int32_t angle_2 = TRIG_MAX_ANGLE;
  graphics_context_set_fill_color(ctx, drawing_data.back_color);
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFillCircle, radius, angle_1, angle_2);
}

// Update the progress ring position based on the current and total values
static void prv_progress_ring_update(void) {
  // calculate new angle
  int32_t new_angle = TRIG_MAX_ANGLE * drawing_data.current_value / drawing_data.total_value;
  // check if large angle and animate
  animation_stop(&drawing_data.progress_angle);
  if (abs(new_angle - drawing_data.progress_angle) >= ANGLE_CHANGE_ANI_THRESHOLD) {
    animation_int32_start(&drawing_data.progress_angle, new_angle, PROGRESS_ANI_DURATION, 0);
  } else {
    drawing_data.progress_angle = new_angle;
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Draw main text onto drawing context
static void prv_render_main_text(GContext *ctx, GRect bounds) {
  // calculate time parts
  int hr = drawing_data.current_value / MSEC_IN_HR;
  int min = drawing_data.current_value % MSEC_IN_HR / MSEC_IN_MIN;
  int sec = drawing_data.current_value % MSEC_IN_MIN / MSEC_IN_SEC;
  // convert to strings
  char hr_buff[4], min_buff[3], sec_buff[3], tot_buff[6];
  snprintf(hr_buff, sizeof(hr_buff), "%d", hr);
  snprintf(min_buff, sizeof(min_buff), hr ? "%02d" : "%d", min);
  snprintf(sec_buff, sizeof(sec_buff), "%02d", sec);
  snprintf(tot_buff, sizeof(tot_buff), "%d:%02d", min, sec);
  // calculate bounds of main text elements
  uint16_t font_size = text_render_get_max_font_size(tot_buff, MAIN_TEXT_BOUNDS);
  GRect min_bounds = text_render_get_content_bounds(min_buff, font_size);
  GRect col_bounds = text_render_get_content_bounds(":", font_size);
  GRect sec_bounds = text_render_get_content_bounds(sec_buff, font_size);
  GRect tot_bounds;
  tot_bounds.size.w = min_bounds.size.w + col_bounds.size.w + sec_bounds.size.w;
  tot_bounds.size.h = min_bounds.size.h;
  tot_bounds.origin.x = (bounds.size.w - tot_bounds.size.w) / 2;
  tot_bounds.origin.y = (bounds.size.h - tot_bounds.size.h) / 2;
  min_bounds.origin = tot_bounds.origin;
  col_bounds.origin.x = min_bounds.origin.x + min_bounds.size.w;
  col_bounds.origin.y = min_bounds.origin.y;
  sec_bounds.origin.x = col_bounds.origin.x + col_bounds.size.w;
  sec_bounds.origin.y = col_bounds.origin.y;
  // draw the main text elements in their respective bounds
  text_render_draw_text(ctx, min_buff, font_size, min_bounds.origin);
  text_render_draw_text(ctx, ":", font_size, col_bounds.origin);
  text_render_draw_text(ctx, sec_buff, font_size, sec_bounds.origin);
}

// Animation update callback
static void prv_animation_update_callback(void) {
  // refresh
  layer_mark_dirty(drawing_data.layer);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Implementation
//

// Set the current timer value used when drawing
void drawing_set_current_value(int64_t current_value) {
  drawing_data.current_value = current_value;
  prv_progress_ring_update();
}

// Set the total timer value used when drawing
void drawing_set_total_value(int64_t total_value) {
  drawing_data.total_value = total_value;
  prv_progress_ring_update();
}

// Render everything to the screen
void drawing_render(Layer *layer, GContext *ctx) {
  // get layer bounds
  GRect bounds = layer_get_bounds(layer);
  // draw background
  // this is actually the ring, which is then covered up with the background
  graphics_context_set_fill_color(ctx, drawing_data.ring_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  prv_render_progress_ring(ctx, bounds);
  // draw main circle
  graphics_context_set_fill_color(ctx, drawing_data.mid_color);
  graphics_fill_circle(ctx, grect_center_point(&bounds), CIRCLE_RADIUS);
  // draw main text (drawn as filled and stroked path)
  graphics_context_set_stroke_color(ctx, drawing_data.fore_color);
  graphics_context_set_fill_color(ctx, drawing_data.fore_color);
  prv_render_main_text(ctx, bounds);
}

// Initialize the singleton drawing data
void drawing_initialize(Layer *layer) {
  // set the layer
  drawing_data.layer = layer;
  // set visual states
  drawing_data.progress_angle = 0;
  // set the values
  drawing_set_current_value(200000);
  drawing_set_total_value(300000);
  // set the colors
  drawing_data.fore_color = GColorBlack;
  drawing_data.mid_color = PBL_IF_COLOR_ELSE(GColorPastelYellow, GColorWhite);
  drawing_data.ring_color = PBL_IF_COLOR_ELSE(GColorOrange, GColorWhite);
  drawing_data.back_color = PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack);
  // set animation update callback
  animation_register_update_callback(&prv_animation_update_callback);
}

// Destroy the singleton drawing data
void drawing_terminate(void) {
  animation_stop_all();
}
