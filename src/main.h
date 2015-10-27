//! @file main.c
//! @brief Main logic for Timer++
//!
//! Contains the main logic code and is responsible for loading
//! and unloading the program
//!
//! @author Eric D. Phillips
//! @date August 27, 2015
//! @bugs No known bugs

#pragma once
#include <pebble.h>


// Current timer mode
typedef enum {
  TimerModeEditHr,
  TimerModeEditMin,
  TimerModeEditSec,
  TimerModeCounting
} TimerMode;


//! Get the current timer value in milliseconds
//! @return The current value of the timer in milliseconds
int64_t main_get_timer_value(void);

//! Get the total timer duration in milliseconds
//! @return The total timer duration in milliseconds
int64_t main_get_timer_length(void);

//! Get whether it is in stopwatch mode
//! @return True if in stopwatch mode, else false
bool main_is_stopwatch_mode(void);

//! Get the current control mode of the timer
//! @return The current TimerMode control mode of the timer
TimerMode main_get_timer_mode(void);
