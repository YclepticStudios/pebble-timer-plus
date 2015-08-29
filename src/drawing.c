// @file drawing.h
// @brief Main drawing code
//
// Contains all the drawing code for this app.
//
// @author Eric D. Phillips
// @date August 29, 2015
// @bugs No known bugs

#include <pebble.h>

// Main data
static struct {
  int64_t     current_value;      //< The current timer time value (milliseconds)
  int64_t     total_value;        //< The total timer time value (milliseconds)

  GColor      fore_color;         //< Color of ring
  GColor      mid_color;          //< Color of center
  GColor      back_color;         //< Color behind ring
} drawing_data;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Get a point from a center point, angle, and radius
static GPoint prv_polar_to_rectangular(GPoint center, uint32_t angle, int16_t radius) {
  return GPoint((-sin_lookup(angle) * radius / TRIG_MAX_RATIO) + center.x,
                (-cos_lookup(angle) * radius / TRIG_MAX_RATIO) + center.y);
}

// Draw the gray cover over part of the progress ring
static void prv_render_progress_ring_cover(GContext *ctx, GRect parent_bounds, uint32_t angle) {
  // get step angle and exit if too small
  int32_t step = angle / 4;
  if (step < 1) {
    return;
  }
  // get properties
  GPoint center = GPoint(parent_bounds.size.w / 2, parent_bounds.size.h / 2);
  int16_t radius = parent_bounds.size.w;
  // calculate points around outside of window to draw cover
  GPoint points[8];
  uint32_t idx = 0;
  for (uint32_t t_angle = 0; t_angle < angle; t_angle += step){
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


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Implementation
//

// Set the current timer value used when drawing
void drawing_set_current_value(int64_t current_value) {
  drawing_data.current_value = current_value;
}

// Set the total timer value used when drawing
void drawing_set_total_value(int64_t total_value) {
  drawing_data.total_value = total_value;
}

// Render everything to the screen
void drawing_render(Layer *layer, GContext *ctx) {
  // get layer bounds
  GRect bounds = layer_get_bounds(layer);
  // draw background
  // this is actually the ring, which is then covered up with the background
  graphics_context_set_fill_color(ctx, drawing_data.fore_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  // draw ring as a cover
  graphics_context_set_fill_color(ctx, drawing_data.back_color);
  prv_render_progress_ring_cover(ctx, bounds, TRIG_MAX_ANGLE * drawing_data.current_value /
    drawing_data.total_value);
}

// Initialize the singleton drawing data
void drawing_initialize(void) {
  // set the values
  drawing_set_current_value(100000);
  drawing_set_total_value(300000);
  // set the colors
  drawing_data.fore_color = COLOR_FALLBACK(GColorOrange, GColorWhite);
  drawing_data.mid_color = COLOR_FALLBACK(GColorPastelYellow, GColorWhite);
  drawing_data.back_color = COLOR_FALLBACK(GColorDarkGray, GColorBlack);
}
