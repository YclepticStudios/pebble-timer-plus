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

// Drawing constants
#define CIRCLE_RADIUS 60
#define ANGLE_CHANGE_ANI_THRESHOLD 348
#define PROGRESS_ANI_DURATION 500

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

// Get a point from a center point, angle, and radius
static GPoint prv_polar_to_rectangular(GPoint center, int32_t angle, int16_t radius) {
  return GPoint((-sin_lookup(angle) * radius / TRIG_MAX_RATIO) + center.x,
                (-cos_lookup(angle) * radius / TRIG_MAX_RATIO) + center.y);
}

// Draw the gray cover over part of the progress ring
static void prv_render_progress_ring_cover(GContext *ctx, GRect bounds, int32_t angle) {
  // get step angle and exit if too small
  int32_t step = angle / 4;
  if (step < 1) {
    return;
  }
  // get properties
  GPoint center = grect_center_point(&bounds);
  int16_t radius = (bounds.size.w + bounds.size.h) / 2;
  // calculate points around outside of window to draw cover
  GPoint points[8];
  uint32_t idx = 0;
  for (int32_t t_angle = 0; t_angle < angle; t_angle += step){
    points[idx++] = prv_polar_to_rectangular(center, t_angle, radius);
  }
  // add point at hand position, and in center (to form pie wedge)
  points[idx++] = prv_polar_to_rectangular(center, angle, radius);
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
  // draw ring as a cover
  graphics_context_set_fill_color(ctx, drawing_data.back_color);
  prv_render_progress_ring_cover(ctx, bounds, drawing_data.progress_angle);
  // draw main circle
  graphics_context_set_fill_color(ctx, drawing_data.mid_color);
  graphics_fill_circle(ctx, grect_center_point(&bounds), CIRCLE_RADIUS);
}

// Initialize the singleton drawing data
void drawing_initialize(Layer *layer) {
  // set the layer
  drawing_data.layer = layer;
  // set visual states
  drawing_data.progress_angle = 0;
  // set the values
  drawing_set_current_value(100000);
  drawing_set_total_value(300000);
  // set the colors
  drawing_data.fore_color = GColorBlack;
  drawing_data.mid_color = COLOR_FALLBACK(GColorPastelYellow, GColorWhite);
  drawing_data.ring_color = COLOR_FALLBACK(GColorOrange, GColorWhite);
  drawing_data.back_color = COLOR_FALLBACK(GColorDarkGray, GColorBlack);
  // set animation update callback
  animation_register_update_callback(&prv_animation_update_callback);
}

// Destroy the singleton drawing data
void drawing_terminate(void) {
  animation_stop_all();
}
