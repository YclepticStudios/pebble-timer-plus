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

// Main data structure
static struct {
  Window    *window;      //< The base window for the application
  Layer     *layer;       //< The base layer on which everything will be drawn

  int64_t   timer_start;  //< The millisecond epoch time at which the timer was started
  int64_t   timer_length; //< The total duration of the timer in milliseconds

  TimerMode timer_mode;   //< The current control mode of the timer
} main_data;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Callbacks
//

// Get the current timer value in milliseconds
int64_t main_get_timer_value(void) {
  // returns the current timer value assuming the following conditions
  // 1. when the timer is running, timer_start represents the epoch when it was started
  // 2. when it is paused, timer_start represents the negative of the time is has been running
  return main_data.timer_length - epoch() + (((main_data.timer_start + epoch() - 1) % epoch()) + 1);
}

// Get the total timer duration in milliseconds
int64_t main_get_timer_length(void) {
  return main_data.timer_length;
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

// Up click handler
static void prv_up_click_handler(ClickRecognizerRef recognizer, void *ctx) {

}

// Select click handler
static void prv_select_click_handler(ClickRecognizerRef recognizer, void *ctx) {
  main_data.timer_mode++;
  if (main_data.timer_mode > TimerModeEditSec) {
    main_data.timer_mode = TimerModeCounting;
  }
  // refresh
  layer_mark_dirty(main_data.layer);
}

// Down click handler
static void prv_down_click_handler(ClickRecognizerRef recognizer, void *ctx) {

}

// Click configuration provider
static void prv_click_config_provider(void *ctx) {
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
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
  main_data.timer_start = -123000;
  main_data.timer_length = 400000;
  main_data.timer_mode = TimerModeCounting;
  drawing_update_progress_ring_angle();

  // initialize drawing singleton
  drawing_initialize(main_data.layer);
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
