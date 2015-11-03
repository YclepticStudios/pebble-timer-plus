// @file main.c
// @brief Main logic for Timer++
//
// Contains the higher level logic code
//
// @author Eric D. Phillips
// @date August 27, 2015
// @bugs No known bugs

#include <pebble.h>
#include "main.h"
#include "drawing.h"
#include "timer.h"
#include "utility.h"

// Main constants
#define BUTTON_HOLD_REPEAT_MS 100

// Main data structure
static struct {
  Window      *window;      //< The base window for the application
  Layer       *layer;       //< The base layer on which everything will be drawn
  ControlMode control_mode; //< The current control mode of the timer
  AppTimer    *app_timer;   //< The AppTimer to keep the screen refreshing
} main_data;

// Function declarations
static void prv_app_timer_callback(void *data);


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Rewind timer if button is clicked to stop vibration
static bool main_timer_rewind(void) {
  // check if timer is vibrating
  if (timer_is_vibrating()) {
    vibes_cancel();
    main_data.control_mode = ControlModeEditSec;
    timer_rewind();
    drawing_update();
    return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Callbacks
//

// Get the current control mode of the timer
ControlMode main_get_control_mode(void) {
  return main_data.control_mode;
}

// Background layer update procedure
static void prv_layer_update_proc_handler(Layer *layer, GContext *ctx) {
  // render the timer's visuals
  drawing_render(layer, ctx);
}

// Back click handler
static void prv_back_click_handler(ClickRecognizerRef recognizer, void *ctx) {
  // cancel vibrations
  vibes_cancel();
  // get time parts
  uint16_t hr, min, sec;
  timer_get_time_parts(&hr, &min, &sec);
  // change control mode
  if ((hr && main_data.control_mode == ControlModeEditMin) ||
      main_data.control_mode == ControlModeEditSec) {
    main_data.control_mode--;
  } else {
    window_stack_pop(true);
  }
  // refresh
  drawing_update();
  layer_mark_dirty(main_data.layer);
}

// Up click handler
static void prv_up_click_handler(ClickRecognizerRef recognizer, void *ctx) {
  // rewind timer if clicked while timer is going off
  if (main_timer_rewind() || main_data.control_mode == ControlModeCounting) {
    return;
  }
  // increment timer
  int64_t increment;
  if (main_data.control_mode == ControlModeEditHr) {
    increment = MSEC_IN_HR;
  } else if (main_data.control_mode == ControlModeEditMin) {
    increment = MSEC_IN_MIN;
  } else {
    increment = MSEC_IN_SEC;
  }
  timer_increment(increment);
  // animate and refresh
  if (!click_recognizer_is_repeating(recognizer)) {
    drawing_start_bounce_animation(true);
  }
  drawing_update();
  layer_mark_dirty(main_data.layer);
}

// Select click handler
static void prv_select_click_handler(ClickRecognizerRef recognizer, void *ctx) {
  // rewind timer if clicked while timer is going off
  if (main_timer_rewind()) {
    return;
  }
  // change timer mode
  switch (main_data.control_mode) {
    case ControlModeEditHr:
      main_data.control_mode = ControlModeEditMin;
      break;
    case ControlModeEditMin:
      main_data.control_mode = ControlModeEditSec;
      break;
    case ControlModeEditSec:
      main_data.control_mode = ControlModeCounting;
      timer_toggle_play_pause();
      if (!main_data.app_timer) {
        prv_app_timer_callback(NULL);
      }
      break;
    case ControlModeCounting:
      main_data.control_mode = ControlModeEditSec;
      timer_toggle_play_pause();
      break;
  }
  // refresh
  drawing_update();
  layer_mark_dirty(main_data.layer);
}

// Select raw click handler
static void prv_select_raw_click_handler(ClickRecognizerRef recognizer, void *ctx) {
  // stop vibration
  vibes_cancel();
  // animate and refresh
  drawing_start_reset_animation();
  layer_mark_dirty(main_data.layer);
}

// Select long click handler
static void prv_select_long_click_handler(ClickRecognizerRef recognizer, void *ctx) {
  main_data.control_mode = ControlModeEditMin;
  timer_reset();
  // animate and refresh
  drawing_update();
  layer_mark_dirty(main_data.layer);
}

// Down click handler
static void prv_down_click_handler(ClickRecognizerRef recognizer, void *ctx) {
  // rewind timer if clicked while timer is going off
  if (main_timer_rewind() || main_data.control_mode == ControlModeCounting) {
    return;
  }
  // increment timer
  int64_t increment;
  if (main_data.control_mode == ControlModeEditHr) {
    increment = -MSEC_IN_HR;
  } else if (main_data.control_mode == ControlModeEditMin) {
    increment = -MSEC_IN_MIN;
  } else {
    increment = -MSEC_IN_SEC;
  }
  timer_increment(increment);
  // animate and refresh
  if (!click_recognizer_is_repeating(recognizer)) {
    drawing_start_bounce_animation(false);
  }
  drawing_update();
  layer_mark_dirty(main_data.layer);
}

// Click configuration provider
static void prv_click_config_provider(void *ctx) {
  window_single_click_subscribe(BUTTON_ID_BACK, prv_back_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, BUTTON_HOLD_REPEAT_MS,
    prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_raw_click_subscribe(BUTTON_ID_SELECT, prv_select_raw_click_handler, NULL, NULL);
  window_long_click_subscribe(BUTTON_ID_SELECT, BUTTON_HOLD_RESET_MS, prv_select_long_click_handler,
    NULL);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, BUTTON_HOLD_REPEAT_MS,
    prv_down_click_handler);
}

// AppTimer callback
static void prv_app_timer_callback(void *data) {
  // check if timer is complete
  timer_check_elapsed();
  // refresh
  drawing_update();
  layer_mark_dirty(main_data.layer);
  // schedule next call
  main_data.app_timer = NULL;
  if (main_data.control_mode == ControlModeCounting) {
    uint32_t duration = timer_get_value_ms() % MSEC_IN_SEC;
    if (timer_is_chrono()) {
      duration = MSEC_IN_SEC - duration;
    }
    main_data.app_timer = app_timer_register(duration + 1, prv_app_timer_callback, NULL);
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Loading and Unloading
//

// Initialize the program
static void prv_initialize(void) {
  // cancel any existing wakeup events
  wakeup_cancel_all();
  // load timer
  timer_persist_read();
  // set initial states
  if (timer_is_paused()) {
    // get time parts
    uint16_t hr, min, sec;
    timer_get_time_parts(&hr, &min, &sec);
    if (hr) {
      main_data.control_mode = ControlModeEditHr;
    } else {
      main_data.control_mode = ControlModeEditMin;
    }
  } else {
    main_data.control_mode = ControlModeCounting;
  }

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

  // initialize drawing singleton
  drawing_initialize(main_data.layer);
  // start refreshing
  prv_app_timer_callback(NULL);
}

// Terminate the program
static void prv_terminate(void) {
  // schedule wakeup
  if (!timer_is_chrono() && !timer_is_paused()) {
    time_t wakeup_time = (epoch() + timer_get_value_ms()) / MSEC_IN_SEC;
    wakeup_schedule(wakeup_time, 0, true);
  }
  // destroy
  timer_persist_store();
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
