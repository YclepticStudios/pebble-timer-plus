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

#include "drawing.h"
#include "utility.h"

//! Main data structure
static struct {
  Window    *window;    //!< The base window for the application
  Layer     *layer;     //!< The base layer on which everything will be drawn
} main_data;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Callbacks
//

//! Background layer update procedure
//! @param layer A pointer to the layer being redrawn
//! @param ctx A pointer to the graphics context
static void prv_layer_update_proc_handler(Layer *layer, GContext *ctx) {
  // render the timer's visuals
  drawing_render(layer, ctx);
}

//! Up click handler
//! @param recognizer The click recognizer
//! @param ctx Pointer to the click context
static void prv_up_click_handler(ClickRecognizerRef recognizer, void *ctx) {

}

//! Select click handler
//! @param recognizer The click recognizer
//! @param ctx Pointer to the click context
static void prv_select_click_handler(ClickRecognizerRef recognizer, void *ctx) {
  layer_mark_dirty(main_data.layer);
}

//! Down click handler
//! @param recognizer The click recognizer
//! @param ctx Pointer to the click context
static void prv_down_click_handler(ClickRecognizerRef recognizer, void *ctx) {

}

//! Click configuration provider
//! @param ctx Pointer to the click context
static void prv_click_config_provider(void *ctx) {
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Loading and Unloading
//

//! Initialize the program
static void prv_initialize(void) {
  // initialize window
  main_data.window = window_create();
  ASSERT(main_data.window);
  window_set_click_config_provider(main_data.window, prv_click_config_provider);
  Layer *window_root = window_get_root_layer(main_data.window);
  GRect window_bounds = layer_get_bounds(window_root);
  window_stack_push(main_data.window, true);
  // initialize main layer
  main_data.layer = layer_create(window_bounds);
  ASSERT(main_data.layer);
  layer_set_update_proc(main_data.layer, prv_layer_update_proc_handler);
  layer_add_child(window_root, main_data.layer);

  // initialize drawing singleton
  drawing_initialize(main_data.layer);
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
