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

//! Defined constants
#define ASSERT(ptr) assert(ptr, __FILE__, __LINE__)

//! Main data structure
struct MainData {
  Window    *window;    //!< The base window for the application
  Layer     *layer;     //!< The base layer on which everything will be drawn
} main_data;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

//! Check pointer for null and assert
//! @param ptr The pointer to check for NULL
//! @param file A pointer to a string representing the name of the current file
//! @param line An integer representing the current line
static void assert(void *ptr, const char *file, int line) {
  if (ptr) {
    return;
  }
  APP_LOG(APP_LOG_LEVEL_ERROR, "Invalid pointer: (%s:%d)", file, line);
  // assert
  void (*exit)(void) = NULL;
  exit();
}

//! Malloc with built in pointer check
//! @param size The size of the memory to allocate
static void *malloc_check(uint16_t size) {
  void *ptr = malloc(size);
  ASSERT(ptr);
  return ptr;
}


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
