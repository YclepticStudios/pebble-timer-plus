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
#include "main.h"
#include "text_render.h"
#include "utility.h"

// Drawing constants
#define CIRCLE_RADIUS 60
#define ANGLE_CHANGE_ANI_THRESHOLD 348
#define PROGRESS_ANI_DURATION 750
#define MAIN_TEXT_CIRCLE_RADIUS (CIRCLE_RADIUS - 7)
#define MAIN_TEXT_BOUNDS GRect(-MAIN_TEXT_CIRCLE_RADIUS, -MAIN_TEXT_CIRCLE_RADIUS / 2,\
 MAIN_TEXT_CIRCLE_RADIUS * 2, MAIN_TEXT_CIRCLE_RADIUS)
#define MAIN_TEXT_CIRCLE_RADIUS_EDIT (CIRCLE_RADIUS - 15)
#define MAIN_TEXT_BOUNDS_EDIT GRect(-MAIN_TEXT_CIRCLE_RADIUS_EDIT, \
 -MAIN_TEXT_CIRCLE_RADIUS_EDIT / 2, MAIN_TEXT_CIRCLE_RADIUS_EDIT * 2, MAIN_TEXT_CIRCLE_RADIUS_EDIT)
#define TEXT_FIELD_COUNT 5
#define TEXT_FIELD_EDIT_SPACING 7
#define TEXT_FIELD_ANI_DURATION 250
#define FOCUS_FIELD_COUNT 2
#define FOCUS_FIELD_BORDER 5
#define FOCUS_FIELD_PAUSE_RADIUS CIRCLE_RADIUS / 2
#define FOCUS_FIELD_ANI_DURATION 150

// Main drawing state description
typedef struct {
  TimerMode   timer_mode;   //< The timer control mode at that state
  uint8_t     hr_digits;    //< The number of digits used by the hours
  uint8_t     min_digits;   //< The number of digits used by the minutes
} DrawState;

// Main data
static struct {
  Layer       *layer;             //< The main layer being drawn on, used to force a refresh
  int32_t     progress_angle;     //< The current angle of the progress ring
  DrawState   draw_state;         //< An arbitrary description of the main drawing state
  GRect       text_fields[TEXT_FIELD_COUNT];      //< The number of text fields (hr : min : sec)
  GRect       focus_fields[FOCUS_FIELD_COUNT];    //< The number of focus fields (selection layer)
  GColor      fore_color;         //< Color of text
  GColor      mid_color;          //< Color of center
  GColor      ring_color;         //< Color of ring
  GColor      back_color;         //< Color behind ring
} drawing_data;

// Focus layer data


////////////////////////////////////////////////////////////////////////////////////////////////////
// Focus Layer
//

// Update focus layer drawing state
static void prv_focus_layer_update_state(Layer *layer, GRect hr_bounds, GRect min_bounds,
                                         GRect sec_bounds) {
  // get properties
  GRect bounds = layer_get_bounds(layer);
  GRect pause_bounds;
  // check current control mode
  if (main_get_timer_mode() == TimerModeCounting) {
    // calculate the size of the pause icon
    pause_bounds.origin = grect_center_point(&bounds);
    pause_bounds.origin.x -= FOCUS_FIELD_PAUSE_RADIUS;
    pause_bounds.origin.y -= FOCUS_FIELD_PAUSE_RADIUS;
    pause_bounds.size.w = FOCUS_FIELD_PAUSE_RADIUS * 2 / 3;
    pause_bounds.size.h = FOCUS_FIELD_PAUSE_RADIUS * 2;
    // animate first focus rectangle
    animation_grect_start(&drawing_data.focus_fields[0], pause_bounds, FOCUS_FIELD_ANI_DURATION, 0);
    // shift grect for rightmost part of pause symbol
    pause_bounds.origin.x += FOCUS_FIELD_PAUSE_RADIUS * 4 / 3;
    animation_grect_start(&drawing_data.focus_fields[1], pause_bounds, FOCUS_FIELD_ANI_DURATION, 0);
  } else {
    // get final grect
    switch (main_get_timer_mode()) {
      case TimerModeEditHr:
        pause_bounds = hr_bounds;
        break;
      case TimerModeEditMin:
        pause_bounds = min_bounds;
        break;
      case TimerModeEditSec:
        pause_bounds = sec_bounds;
        break;
      default:
        return;
    }
    // add border
    pause_bounds = grect_inset(pause_bounds, GEdgeInsets1(-FOCUS_FIELD_BORDER));
    // animate the focus field to those bounds
    animation_grect_start(&drawing_data.focus_fields[0], pause_bounds, FOCUS_FIELD_ANI_DURATION, 0);
    animation_grect_start(&drawing_data.focus_fields[1], pause_bounds, FOCUS_FIELD_ANI_DURATION, 0);
  }
}

// Draw the focus layer
static void prv_render_focus_layer(GContext *ctx, GRect bounds) {
  graphics_context_set_fill_color(ctx, drawing_data.ring_color);
  graphics_fill_rect(ctx, drawing_data.focus_fields[0], 0, GCornerNone);
  graphics_fill_rect(ctx, drawing_data.focus_fields[1], 0, GCornerNone);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Main Text
//

// Update main text drawing state
static void prv_main_text_update_state(Layer *layer) {
  // get properties
  GRect bounds = layer_get_bounds(layer);
  bool edit_mode = main_get_timer_mode() != TimerModeCounting;
  // calculate time parts
  int64_t timer_value = main_get_timer_value();
  int hr = timer_value / MSEC_IN_HR;
  int min = timer_value % MSEC_IN_HR / MSEC_IN_MIN;
  int sec = timer_value % MSEC_IN_MIN / MSEC_IN_SEC;
  // convert to strings
  char buff[TEXT_FIELD_COUNT][4];
  bool edit = main_get_timer_mode() != TimerModeCounting;
  if (hr) {
    snprintf(buff[0], sizeof(buff[0]), edit ? "%02d" : "%d", hr);
  } else {
    snprintf(buff[0], sizeof(buff[0]), "%s", "\0");
  }
  snprintf(buff[1], sizeof(buff[1]), "%s", hr && !edit ? ":" : "\0");
  snprintf(buff[2], sizeof(buff[2]), (hr || edit) ? "%02d" : "%d", min);
  snprintf(buff[3], sizeof(buff[3]), "%s", edit ? "\0" : ":");
  snprintf(buff[4], sizeof(buff[4]), "%02d", sec);
  // calculate new sizes for all text elements
  char tot_buff[8];
  snprintf(tot_buff, sizeof(tot_buff), "%s%s%s%s%s", buff[0], buff[1], buff[2], buff[3], buff[4]);
  uint16_t font_size = text_render_get_max_font_size(tot_buff, edit_mode ? MAIN_TEXT_BOUNDS_EDIT :
                                                               MAIN_TEXT_BOUNDS);
  // calculate new size for each text element
  GRect total_bounds = GRectZero;
  GRect field_bounds[TEXT_FIELD_COUNT];
  for (uint8_t ii = 0; ii < TEXT_FIELD_COUNT; ii++) {
    field_bounds[ii] = text_render_get_content_bounds(buff[ii], font_size);
    // if in edit mode and some fields have content and this one is '\0', then pad it
    if (edit_mode && total_bounds.size.w && field_bounds[ii].size.w == 0) {
      field_bounds[ii].size.w = TEXT_FIELD_EDIT_SPACING;
    }
    total_bounds.size.w += field_bounds[ii].size.w;
  }
  total_bounds.size.h = field_bounds[TEXT_FIELD_COUNT - 1].size.h;
  total_bounds.origin.x = (bounds.size.w - total_bounds.size.w) / 2;
  total_bounds.origin.y = (bounds.size.h - total_bounds.size.h) / 2;
  // calculate positions for all text elements
  field_bounds[0].origin = total_bounds.origin;
  for (uint8_t ii = 0; ii < TEXT_FIELD_COUNT - 1; ii++) {
    field_bounds[ii + 1].origin.x = field_bounds[ii].origin.x + field_bounds[ii].size.w;
    field_bounds[ii + 1].origin.y = total_bounds.origin.y;
  }
  // animate to new positions
  for (uint8_t ii = 0; ii < TEXT_FIELD_COUNT; ii++) {
    animation_grect_start(&drawing_data.text_fields[ii], field_bounds[ii],
                          TEXT_FIELD_ANI_DURATION, 0);
  }

  // update the focus layers
  prv_focus_layer_update_state(layer, field_bounds[0], field_bounds[2], field_bounds[4]);
}

// Draw main text onto drawing context
static void prv_render_main_text(GContext *ctx, GRect bounds) {
  // calculate time parts
  int64_t timer_value = main_get_timer_value();
  int hr = timer_value / MSEC_IN_HR;
  int min = timer_value % MSEC_IN_HR / MSEC_IN_MIN;
  int sec = timer_value % MSEC_IN_MIN / MSEC_IN_SEC;
  // convert to strings
  char buff[TEXT_FIELD_COUNT][4];
  bool edit = main_get_timer_mode() != TimerModeCounting;
  if (hr) {
    snprintf(buff[0], sizeof(buff[0]), edit ? "%02d" : "%d", hr);
  } else {
    snprintf(buff[0], sizeof(buff[0]), "%s", "\0");
  }
  snprintf(buff[1], sizeof(buff[1]), "%s", hr && !edit ? ":" : "\0");
  snprintf(buff[2], sizeof(buff[2]), (hr || edit) ? "%02d" : "%d", min);
  snprintf(buff[3], sizeof(buff[3]), "%s", edit ? "\0" : ":");
  snprintf(buff[4], sizeof(buff[4]), "%02d", sec);
  // draw the main text elements in their respective bounds
  for (uint8_t ii = 0; ii < TEXT_FIELD_COUNT; ii++) {
    text_render_draw_scalable_text(ctx, buff[ii], drawing_data.text_fields[ii]);
  }
}

// Animation update callback
static void prv_animation_update_callback(void) {
  // refresh
  layer_mark_dirty(drawing_data.layer);
}


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
  int32_t new_angle = TRIG_MAX_ANGLE * main_get_timer_value() / main_get_timer_length();
  // check if large angle and animate
  animation_stop(&drawing_data.progress_angle);
  if (abs(new_angle - drawing_data.progress_angle) >= ANGLE_CHANGE_ANI_THRESHOLD) {
    animation_int32_start(&drawing_data.progress_angle, new_angle, PROGRESS_ANI_DURATION, 0);
  } else {
    drawing_data.progress_angle = new_angle;
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Drawing State Changes
//

// Compare two different TextStates, return true if same
static bool prv_text_state_compare(DrawState text_state_1, DrawState text_state_2) {
  return text_state_1.timer_mode == text_state_2.timer_mode &&
         text_state_1.hr_digits == text_state_2.hr_digits &&
         text_state_1.min_digits == text_state_2.min_digits;
}

// Create a state description
static DrawState prv_draw_state_create(TimerMode timer_mode, int64_t timer_current) {
  uint8_t hours = timer_current / MSEC_IN_HR;
  uint8_t minutes = timer_current % MSEC_IN_HR / MSEC_IN_MIN;
  return (DrawState) {
    .timer_mode = timer_mode,
    .hr_digits = (hours + 9) / 10,
    .min_digits = (minutes + 9) / 10,
  };
}

// Check for draw state changes and update drawing accordingly
static void prv_update_draw_state(Layer *layer) {
  // check for changes in the states of things
  DrawState cur_text_state = prv_draw_state_create(main_get_timer_mode(), main_get_timer_value());
  if (!prv_text_state_compare(cur_text_state, drawing_data.draw_state)) {
    // set the old state to the new one
    drawing_data.draw_state = cur_text_state;
    // update drawing functions
    // focus update called in main text update
    prv_main_text_update_state(layer);
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Implementation
//

// Update the progress ring angle based on the timer values
void drawing_update_progress_ring_angle(void) {
  prv_progress_ring_update();
}

// Render everything to the screen
void drawing_render(Layer *layer, GContext *ctx) {
  // check for drawing state changes
  prv_update_draw_state(layer);

  // get properties
  GRect bounds = layer_get_bounds(layer);
  // draw background
  // this is actually the ring, which is then covered up with the background
  graphics_context_set_fill_color(ctx, drawing_data.ring_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  prv_render_progress_ring(ctx, bounds);
  // draw main circle
  graphics_context_set_fill_color(ctx, drawing_data.mid_color);
  graphics_fill_circle(ctx, grect_center_point(&bounds), CIRCLE_RADIUS);
  // draw focus layer
  prv_render_focus_layer(ctx, bounds);
  // draw main text (drawn as filled and stroked path)
  graphics_context_set_stroke_color(ctx, drawing_data.fore_color);
  graphics_context_set_fill_color(ctx, drawing_data.fore_color);
  prv_render_main_text(ctx, bounds);
}

// Initialize the singleton drawing data
void drawing_initialize(Layer *layer) {
  // get properties
  GRect bounds = layer_get_bounds(layer);
  // set the layer
  drawing_data.layer = layer;
  // set visual states
  drawing_data.progress_angle = 0;
  for (uint8_t ii = 0; ii < TEXT_FIELD_COUNT; ii++) {
    drawing_data.text_fields[ii].origin = grect_center_point(&bounds);
    drawing_data.text_fields[ii].size = GSizeZero;
  }
  for (uint8_t ii = 0; ii < FOCUS_FIELD_COUNT; ii++) {
    drawing_data.focus_fields[ii].origin = grect_center_point(&bounds);
    drawing_data.focus_fields[ii].size = GSizeZero;
  }
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
