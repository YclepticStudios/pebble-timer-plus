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
#include "timer.h"
#include "utility.h"

// Drawing constants
// Progress ring
#define CIRCLE_RADIUS 63
#define ANGLE_CHANGE_ANI_THRESHOLD 348
#define PROGRESS_ANI_DURATION 250
#define MAIN_TEXT_CIRCLE_RADIUS (CIRCLE_RADIUS - 7)
#define MAIN_TEXT_BOUNDS GRect(-MAIN_TEXT_CIRCLE_RADIUS, -MAIN_TEXT_CIRCLE_RADIUS / 2,\
 MAIN_TEXT_CIRCLE_RADIUS * 2, MAIN_TEXT_CIRCLE_RADIUS)
#define MAIN_TEXT_CIRCLE_RADIUS_EDIT (CIRCLE_RADIUS - 17)
#define MAIN_TEXT_BOUNDS_EDIT GRect(-MAIN_TEXT_CIRCLE_RADIUS_EDIT, \
 -MAIN_TEXT_CIRCLE_RADIUS_EDIT / 2, MAIN_TEXT_CIRCLE_RADIUS_EDIT * 2, MAIN_TEXT_CIRCLE_RADIUS_EDIT)
// Main Text
#define TEXT_FIELD_COUNT 5
#define TEXT_FIELD_EDIT_SPACING 7
#define TEXT_FIELD_ANI_DURATION 140
// Focus Layer
#define FOCUS_FIELD_BORDER 5
#define FOCUS_FIELD_SHRINK_INSET 3
#define FOCUS_FIELD_SHRINK_DURATION 80
#define FOCUS_FIELD_ANI_DURATION 150
#define FOCUS_BOUNCE_ANI_HEIGHT 8
#define FOCUS_BOUNCE_ANI_DURATION 70
#define FOCUS_BOUNCE_ANI_SETTLE_DURATION 140
// Header Text
#define HEADER_Y_OFFSET 5
#define FOOTER_Y_OFFSET -39

// Main drawing state description, used to determine changes in state
typedef struct {
  ControlMode   control_mode;   //< The timer control mode at that state
  uint8_t       hr_digits;      //< The number of digits used by the hours
  uint8_t       min_digits;     //< The number of digits used by the minutes
} DrawState;

// Main data
static struct {
  Layer       *layer;             //< The main layer being drawn on, used to force a refresh
  int32_t     progress_angle;     //< The current angle of the progress ring
  DrawState   draw_state;         //< An arbitrary description of the main drawing state
  GRect       text_fields[TEXT_FIELD_COUNT];      //< The number of text fields (hr : min : sec)
  GRect       focus_field;        //< The selection field layer
  GColor      fore_color;         //< Color of text
  GColor      mid_color;          //< Color of center
  GColor      ring_color;         //< Color of ring
  GColor      back_color;         //< Color behind ring
} drawing_data;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Focus Layer
//

// Update focus layer drawing state
static void prv_focus_layer_update_state(Layer *layer, GRect hr_bounds, GRect min_bounds,
                                         GRect sec_bounds) {
  // get properties
  GRect bounds = layer_get_bounds(layer);
  // check current control mode
  if (main_get_control_mode() == ControlModeCounting) {
    // calculate the bounds for the focus layer off the screen
    bounds.origin.x = bounds.size.w;
    bounds.origin.y = bounds.size.h / 2 - sec_bounds.size.h / 4;
    bounds.size.w = sec_bounds.size.w;
    bounds.size.h = sec_bounds.size.h / 2;
    // animate the focus layer
    animation_grect_start(&drawing_data.focus_field, bounds, FOCUS_FIELD_ANI_DURATION, 0,
      CurveSinEaseOut);
  } else {
    // get final bounds when in editing mode
    if (main_get_control_mode() == ControlModeEditHr) {
      bounds = hr_bounds;
    } else if (main_get_control_mode() == ControlModeEditMin) {
      bounds = min_bounds;
    } else {
      bounds = sec_bounds;
    }
    // add border
    bounds = grect_inset(bounds, GEdgeInsets1(-FOCUS_FIELD_BORDER));
    // animate the focus field to those bounds
    animation_grect_start(&drawing_data.focus_field, bounds, FOCUS_FIELD_ANI_DURATION, 0,
      CurveSinEaseOut);
  }
}

// Draw the focus layer
static void prv_render_focus_layer(GContext *ctx) {
#ifdef PBL_SDK_2
  graphics_fill_rect_grey(ctx, drawing_data.focus_field);
#else
  graphics_context_set_fill_color(ctx, drawing_data.ring_color);
  graphics_fill_rect(ctx, drawing_data.focus_field, 0, GCornerNone);
#endif
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Sub Texts
//

// Draw header text
static void prv_render_header_text(GContext *ctx, GRect bounds) {
  // calculate bounds
  bounds.origin = grect_center_point(&bounds);
  bounds.origin.x -= CIRCLE_RADIUS;
  bounds.origin.y -= (CIRCLE_RADIUS - HEADER_Y_OFFSET);
  bounds.size.w = CIRCLE_RADIUS * 2;
  bounds.size.h = CIRCLE_RADIUS / 2;
  // draw text
  char *buff;
  if (timer_is_chrono()) {
    buff = "Chrono";
  } else {
    buff = "Timer";
  }
  graphics_draw_text(ctx, buff, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), bounds,
    GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

// Draw footer text
static void prv_render_footer_text(GContext *ctx, GRect bounds) {
  // calculate bounds
  bounds.origin = grect_center_point(&bounds);
  bounds.origin.x -= CIRCLE_RADIUS;
  bounds.origin.y += CIRCLE_RADIUS + FOOTER_Y_OFFSET;
  bounds.size.w = CIRCLE_RADIUS * 2;
  bounds.size.h = CIRCLE_RADIUS / 2;
  // calculate text
  char buff[10];
  // in timer mode, get time
  time_t end_time = epoch() / MSEC_IN_SEC;
  if (main_get_control_mode() != ControlModeCounting && !timer_is_chrono()) {
    end_time += timer_get_value_ms() / MSEC_IN_SEC;
  }
  // format to readable time
  struct tm end_tm = *localtime(&end_time);
  strftime(buff, sizeof(buff), clock_is_24h_style() ? "%k:%M" : "%l:%M", &end_tm);
  // draw text
  graphics_draw_text(ctx, buff, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), bounds,
    GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Main Text
//

// Update main text drawing state
static void prv_main_text_update_state(Layer *layer) {
  // get properties
  GRect bounds = layer_get_bounds(layer);
  bool edit_mode = main_get_control_mode() != ControlModeCounting;
  // calculate time parts
  uint16_t hr, min, sec;
  timer_get_time_parts(&hr, &min, &sec);
  // convert to strings
  char buff[TEXT_FIELD_COUNT][4] = {{'\0'}};
  if (hr) {
    snprintf(buff[0], sizeof(buff[0]), edit_mode ? "%02d" : "%d", hr);
  }
  snprintf(buff[1], sizeof(buff[1]), "%s", hr && !edit_mode ? ":" : "\0");
  snprintf(buff[2], sizeof(buff[2]), (hr || edit_mode) ? "%02d" : "%d", min);
  snprintf(buff[3], sizeof(buff[3]), "%s", edit_mode ? "\0" : ":");
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
      TEXT_FIELD_ANI_DURATION, 0, CurveSinEaseOut);
  }

  // update the focus layers
  prv_focus_layer_update_state(layer, field_bounds[0], field_bounds[2], field_bounds[4]);
}

// Draw main text onto drawing context
static void prv_render_main_text(GContext *ctx, GRect bounds) {
  // get time parts
  uint16_t hr, min, sec;
  timer_get_time_parts(&hr, &min, &sec);
  // convert to strings
  bool edit_mode = main_get_control_mode() != ControlModeCounting;
  char buff[TEXT_FIELD_COUNT][4] = {{'\0'}};
  if (hr) {
    snprintf(buff[0], sizeof(buff[0]), edit_mode ? "%02d" : "%d", hr);
  }
  snprintf(buff[1], sizeof(buff[1]), "%s", hr && !edit_mode ? ":" : "\0");
  snprintf(buff[2], sizeof(buff[2]), (hr || edit_mode) ? "%02d" : "%d", min);
  snprintf(buff[3], sizeof(buff[3]), "%s", edit_mode ? "\0" : ":");
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
  int32_t new_angle = TRIG_MAX_ANGLE * timer_get_value_ms() / timer_get_length_ms();
  if (timer_is_chrono()) {
    new_angle = TRIG_MAX_ANGLE * (timer_get_value_ms() % MSEC_IN_MIN) / MSEC_IN_MIN;
  }
  // check if large angle and animate
  animation_stop(&drawing_data.progress_angle);
  if (abs(new_angle - drawing_data.progress_angle) >= ANGLE_CHANGE_ANI_THRESHOLD) {
    animation_int32_start(&drawing_data.progress_angle, new_angle, PROGRESS_ANI_DURATION, 0,
      CurveSinEaseOut);
  } else {
    drawing_data.progress_angle = new_angle;
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Drawing State Changes
//

// Compare two different TextStates, return true if conditions are met for a refresh
static bool prv_text_state_compare(DrawState text_state_1, DrawState text_state_2) {
  return text_state_1.control_mode == text_state_2.control_mode && // if control modes are different
         ((text_state_1.control_mode != ControlModeCounting &&     // if in edit mode
           ((text_state_1.hr_digits && text_state_2.hr_digits) ||
            (!text_state_1.hr_digits && !text_state_2.hr_digits))) ||
          (text_state_1.control_mode == ControlModeCounting &&     // if in counting mode
           text_state_1.hr_digits == text_state_2.hr_digits &&
           text_state_1.min_digits == text_state_2.min_digits)) &&
          text_state_2.hr_digits < 3; // on first start hr is set to 99 to force refresh
}

// Create a state description
static DrawState prv_draw_state_create(void) {
  // get states
  uint16_t hr, min, sec;
  timer_get_time_parts(&hr, &min, &sec);
  return (DrawState) {
    .control_mode = main_get_control_mode(),
    .hr_digits = (uint8_t)(hr > 0) + (uint8_t)(hr > 9) + (uint8_t)(hr > 99),
    .min_digits = (uint8_t)(min > 0) + (uint8_t)(min > 9),
  };
}

// Check for draw state changes and update drawing accordingly
static void prv_update_draw_state(Layer *layer) {
  // check for changes in the states of things
  DrawState cur_draw_state = prv_draw_state_create();
  if (!prv_text_state_compare(cur_draw_state, drawing_data.draw_state)) {
    drawing_data.draw_state = cur_draw_state;
    // update text state
    prv_main_text_update_state(layer);
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Implementation
//

// Create bounce animation for focus layer
void drawing_start_bounce_animation(bool upward) {
  // get the currently selected elements
  // only animate the position of one focus layer, stacking gives appearance of stretching
  GRect *txt_rect;
  if (main_get_control_mode() == ControlModeEditHr) {
    txt_rect = &drawing_data.text_fields[0];
  } else if (main_get_control_mode() == ControlModeEditMin) {
    txt_rect = &drawing_data.text_fields[2];
  } else {
    txt_rect = &drawing_data.text_fields[4];
  }
  // animate text
  GRect rect_to = (*txt_rect);
  rect_to.origin.y = drawing_data.text_fields[1].origin.y;
  rect_to.origin.y += (upward ? -1 : 1) * FOCUS_BOUNCE_ANI_HEIGHT;
  animation_grect_start(txt_rect, rect_to, FOCUS_BOUNCE_ANI_DURATION, 0, CurveSinEaseIn);
  rect_to.origin.y = drawing_data.text_fields[1].origin.y;
  animation_grect_start(txt_rect, rect_to, FOCUS_BOUNCE_ANI_SETTLE_DURATION,
    FOCUS_BOUNCE_ANI_DURATION, CurveSinEaseOut);
  // animate focus layer
  rect_to = drawing_data.focus_field;
  rect_to.origin.y += (upward ? -1 : 0) * FOCUS_BOUNCE_ANI_HEIGHT;
  rect_to.size.h += FOCUS_BOUNCE_ANI_HEIGHT;
  animation_grect_start(&drawing_data.focus_field, rect_to, FOCUS_BOUNCE_ANI_DURATION,
    FOCUS_BOUNCE_ANI_DURATION, CurveSinEaseIn);
  // return to original position
  // get final bounds when in editing mode
  GRect txt_bounds = drawing_data.text_fields[0];
  if (main_get_control_mode() == ControlModeEditMin) {
    txt_bounds = drawing_data.text_fields[2];
  } else if (main_get_control_mode() == ControlModeEditSec){
    txt_bounds = drawing_data.text_fields[4];
  }
  rect_to = grect_inset(txt_bounds, GEdgeInsets1(-FOCUS_FIELD_BORDER));
  animation_grect_start(&drawing_data.focus_field, rect_to, FOCUS_BOUNCE_ANI_SETTLE_DURATION,
    FOCUS_BOUNCE_ANI_DURATION * 2, CurveSinEaseOut);
}

// Create reset animation for focus layer
void drawing_start_reset_animation(void) {
  // create shrunken focus bounds and animate to new bounds
  GRect focus_to_bounds;
  focus_to_bounds = grect_inset(drawing_data.focus_field,
    GEdgeInsets1(FOCUS_FIELD_SHRINK_INSET));
  // shrinking animation
  animation_grect_start(&drawing_data.focus_field, focus_to_bounds,
    FOCUS_FIELD_SHRINK_DURATION, 0, CurveLinear);
  // return animation back to original size
  animation_grect_start(&drawing_data.focus_field, drawing_data.focus_field,
    FOCUS_FIELD_SHRINK_DURATION, BUTTON_HOLD_RESET_MS, CurveLinear);
}

// Render everything to the screen
void drawing_render(Layer *layer, GContext *ctx) {
  // get properties
  GRect bounds = layer_get_bounds(layer);
  // draw background
  // this is actually the ring, which is then covered up with the background
  graphics_context_set_fill_color(ctx, drawing_data.ring_color);
#ifdef PBL_SDK_2
  graphics_fill_rect_grey(ctx, bounds);
#else
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
#endif
  prv_render_progress_ring(ctx, bounds);
  // draw main circle
  graphics_context_set_fill_color(ctx, drawing_data.mid_color);
  graphics_fill_circle(ctx, grect_center_point(&bounds), CIRCLE_RADIUS);
  // draw focus layer
  prv_render_focus_layer(ctx);
  // draw main text (drawn as filled and stroked path)
  graphics_context_set_stroke_color(ctx, drawing_data.fore_color);
  graphics_context_set_fill_color(ctx, drawing_data.fore_color);
  prv_render_main_text(ctx, bounds);
  // draw header and footer text
  graphics_context_set_text_color(ctx, drawing_data.fore_color);
  prv_render_header_text(ctx, bounds);
  prv_render_footer_text(ctx, bounds);
}

// Update the drawing states and recalculate everythings positions
void drawing_update(void) {
  // update drawing state
  prv_update_draw_state(drawing_data.layer);
  // update progress ring angle
  prv_progress_ring_update();
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
  drawing_data.focus_field.origin = grect_center_point(&bounds);
  drawing_data.focus_field.size = GSizeZero;
  // set initial draw state to something which guaranties a refresh
  drawing_data.draw_state = (DrawState) {
    .hr_digits = 99,
  };
  // set the colors
  drawing_data.fore_color = GColorBlack;
  drawing_data.mid_color = PBL_IF_COLOR_ELSE(GColorMintGreen, GColorWhite);
  drawing_data.ring_color = PBL_IF_COLOR_ELSE(GColorGreen, GColorWhite);
  drawing_data.back_color = PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack);
  // set animation update callback
  animation_register_update_callback(&prv_animation_update_callback);
}

// Destroy the singleton drawing data
void drawing_terminate(void) {
  animation_stop_all();
}
