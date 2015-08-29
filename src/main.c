//! @file main.c
//! @brief Main logic for Timer++
//!
//! Contains the main logic code and is responsible for loading
//! and unloading the program
//!
//! @author Eric D. Phillips
//! @date August 27, 2015
//! @bugs No known bugs

#include <pebble.h>

#include "utility.h"

//! Main data structure
struct MainData {
  Window    *window;    //!< The base window for the application
  Layer     *layer;     //!< The base layer on which everything will be drawn
} main_data;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//


////////////////////////////////////////////////////////////////////////////////////////////////////
// Loading and Unloading
//

//! Initialize the program
static void prv_initialize(void) {
  // initialize window
  main_data.window = window_create();
  ASSERT(main_data.window);
  Layer *window_root = window_get_root_layer(main_data.window);
  GRect window_bounds = layer_get_bounds(window_root);
  window_stack_push(main_data.window, true);
  // initialize main layer
  main_data.layer = layer_create(window_bounds);
  ASSERT(main_data.layer);
  layer_add_child(window_root, main_data.layer);
}

//! Terminate the program
static void prv_terminate(void) {
  layer_destroy(main_data.layer);
  window_destroy(main_data.window);
}

//! Entry point
int main(void) {
  prv_initialize();
  app_event_loop();
  prv_terminate();
}
