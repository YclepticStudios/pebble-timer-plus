// @file main.c
// @brief Main logic for Timer++
//
// Contains the main logic code and is responsible for loading
// and unloading the program
//
// @author Eric D. Phillips
// @date August 27, 2015
// @bugs No known bugs

#include <pebble.h>
#include "main.h"
#include "drawing.h"
#include "utility.h"

// Main constants
#define BUTTON_HOLD_REPEAT_MS 100

// Main data structure
static struct {
  Window    *window;      //< The base window for the application
  Layer     *layer;       //< The base layer on which everything will be drawn

  int64_t   timer_start;  //< The millisecond epoch time at which the timer was started
  int64_t   timer_length; //< The total duration of the timer in milliseconds

  TimerMode timer_mode;   //< The current control mode of the timer
  AppTimer  *app_timer;   //< The AppTimer to keep the screen refreshing
} main_data;

// Function declarations
static void prv_app_timer_callback(void *data);


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Increment and Decrement the timer
static void prv_timer_increment(TimerMode timer_mode, bool up) {
  // if in stopwatch mode, reset to zero
  if (main_is_stopwatch_mode()) {
    main_data.timer_start = 0;
    return;
  }

  // get increment to change by
  int64_t increment, interval;
  switch (timer_mode) {
    case TimerModeEditHr:
      increment = MSEC_IN_HR;
      interval = 100 * MSEC_IN_HR;
      break;
    case TimerModeEditMin:
      increment = MSEC_IN_MIN;
      interval = MSEC_IN_HR;
      break;
    case TimerModeEditSec:
      increment = MSEC_IN_SEC;
      interval = MSEC_IN_MIN;
      break;
    default:
      return;
  }
  if (!up) {
    increment = -increment;
  }

  // calculate new time by incrementing with wrapping
  int64_t ls_bit = (main_data.timer_length + main_data.timer_start) % interval;
  int64_t step = (ls_bit + interval + increment) % interval - ls_bit;
  if (main_data.timer_start) {
    main_data.timer_start += step;
    if (main_data.timer_start > 0) {
      main_data.timer_length += main_data.timer_start;
      main_data.timer_start = 0;
    }
  } else {
    main_data.timer_length += step;
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Callbacks
//

// Get the current timer value in milliseconds
int64_t main_get_timer_value(void) {
  // returns the current timer value assuming the following conditions
  // 1. when the timer is running, timer_start represents the epoch when it was started
  // 2. when it is paused, timer_start represents the negative of the time is has been running
  int64_t value = main_data.timer_length - epoch() +
    (((main_data.timer_start + epoch() - 1) % epoch()) + 1);
  if (value < 0) {
    value = -value;
  }
  return value;
}

// Get the total timer duration in milliseconds
int64_t main_get_timer_length(void) {
  return main_data.timer_length;
}

// Get whether or not it is in stopwatch mode
bool main_is_stopwatch_mode(void) {
  int64_t value = main_data.timer_length - epoch() +
    (((main_data.timer_start + epoch() - 1) % epoch()) + 1);
  if (value < 0) {
    return true;
  }
  return false;
}

// Get the current control mode of the timer
TimerMode main_get_timer_mode(void) {
  return main_data.timer_mode;
}

// Background layer update procedure
static void prv_layer_update_proc_handler(Layer *layer, GContext *ctx) {
  // render the timer's visuals
  drawing_render(layer, ctx);
}

// Back click handler
static void prv_back_click_handler(ClickRecognizerRef recognizer, void *ctx) {
  // change timer mode
  if (main_data.timer_mode == TimerModeEditHr || main_data.timer_mode == TimerModeCounting ||
      (main_data.timer_length / MSEC_IN_HR == 0 && main_data.timer_mode == TimerModeEditMin)) {
    window_stack_pop(true);
  } else {
    main_data.timer_mode--;
  }
  // refresh
  layer_mark_dirty(main_data.layer);
}

// Up click handler
static void prv_up_click_handler(ClickRecognizerRef recognizer, void *ctx) {
  // increment timer
  prv_timer_increment(main_data.timer_mode, true);
  // animate and refresh
  if (!click_recognizer_is_repeating(recognizer)) {
    drawing_start_bounce_animation(true);
  }
  layer_mark_dirty(main_data.layer);
}

// Select click handler
static void prv_select_click_handler(ClickRecognizerRef recognizer, void *ctx) {
  // change timer mode
  switch (main_data.timer_mode) {
    case TimerModeCounting:
      main_data.timer_mode = TimerModeEditSec;
      main_data.timer_start -= epoch();
      break;
    case TimerModeEditHr:
      main_data.timer_mode = TimerModeEditMin;
      break;
    case TimerModeEditMin:
      main_data.timer_mode = TimerModeEditSec;
      break;
    case TimerModeEditSec:
      main_data.timer_mode = TimerModeCounting;
      main_data.timer_start += epoch();
      if (!main_data.app_timer) {
        prv_app_timer_callback(NULL);
      }
      break;
  }
  // refresh
  layer_mark_dirty(main_data.layer);
}

// Down click handler
static void prv_down_click_handler(ClickRecognizerRef recognizer, void *ctx) {
  // increment timer
  prv_timer_increment(main_data.timer_mode, false);
  // animate and refresh
  if (!click_recognizer_is_repeating(recognizer)) {
    drawing_start_bounce_animation(false);
  }
  layer_mark_dirty(main_data.layer);
}

// Click configuration provider
static void prv_click_config_provider(void *ctx) {
  window_single_click_subscribe(BUTTON_ID_BACK, prv_back_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, BUTTON_HOLD_REPEAT_MS,
    prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, BUTTON_HOLD_REPEAT_MS,
    prv_down_click_handler);
}

// AppTimer callback
static void prv_app_timer_callback(void *data) {
  // refresh
  layer_mark_dirty(main_data.layer);
  drawing_update_progress_ring_angle();
  // schedule next call
  main_data.app_timer = NULL;
  if (main_data.timer_mode == TimerModeCounting) {
    uint32_t duration = main_get_timer_value() % MSEC_IN_SEC + 1;
    main_data.app_timer = app_timer_register(duration, prv_app_timer_callback, NULL);
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Loading and Unloading
//

// Initialize the program
static void prv_initialize(void) {
  // initialize window
  main_data.window = window_create();
  ASSERT(main_data.window);
  window_set_click_config_provider(main_data.window, prv_click_config_provider);
  Layer *window_root = window_get_root_layer(main_data.window);
  GRect window_bounds = layer_get_bounds(window_root);
#ifdef PBL_SDK_2
  window_set_fullscreen(main_data.window, true);
  window_bounds.size.h = 168;
#endif
  window_stack_push(main_data.window, true);
  // initialize main layer
  main_data.layer = layer_create(window_bounds);
  ASSERT(main_data.layer);
  layer_set_update_proc(main_data.layer, prv_layer_update_proc_handler);
  layer_add_child(window_root, main_data.layer);

  // set dummy timer values
  main_data.timer_start = 0;
  main_data.timer_length = 300000;
  main_data.timer_mode = TimerModeEditMin;
  drawing_update_progress_ring_angle();

  // initialize drawing singleton
  drawing_initialize(main_data.layer);
  // start refreshing
  prv_app_timer_callback(NULL);
}

// Terminate the program
static void prv_terminate(void) {
  drawing_terminate();
  layer_destroy(main_data.layer);
  window_destroy(main_data.window);
}

// Entry point
int main(void) {
  prv_initialize();
  app_event_loop();
  prv_terminate();
}
