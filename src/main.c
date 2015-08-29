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
  // create the window
}

//! Terminate the program
static void prv_terminate(void) {

}

//! Entry point
uint8_t main(void) {
  prv_initialize();
  app_event_loop();
  prv_terminate();
}
