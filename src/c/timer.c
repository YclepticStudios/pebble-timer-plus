// @file timer.c
// @brief Data and controls for timer
//
// Contains data and all functions for setting and accessing
// a timer. Also saves and loads timers between closing and reopening.
//
// @author Eric D. Phillips
// @data October 26, 2015
// @bugs No known bugs

#include "timer.h"
#include "utility.h"

#define PERSIST_VERSION 2
#define PERSIST_VERSION_KEY 4342896
#define PERSIST_TIMER_KEY 58734
#define VIBRATION_LENGTH_MS 20000
// legacy persistent storage
#define PERSIST_TIMER_KEY_V2 3456

// Vibration sequence
static const uint32_t vibe_sequence[] = {150, 200, 300};
static const VibePattern vibe_pattern = {
  .durations = vibe_sequence,
  .num_segments = ARRAY_LENGTH(vibe_sequence),
};

// Main data structure
typedef struct {
  int64_t     length_ms;      //< Length of timer in milliseconds
  int64_t     start_ms;       //< The start epoch of the timer in milliseconds
  bool        elapsed;        //< Used to start the vibration if first time as elapsed
  bool        can_vibrate;    //< Flag used to tell when the timer has completed
} Timer;
Timer timer_data;


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Functions
//

// Get timer value divided into time parts
void timer_get_time_parts(uint16_t *hr, uint16_t *min, uint16_t *sec) {
  int64_t value = timer_get_value_ms();
  (*hr) = value / MSEC_IN_HR;
  (*min) = value % MSEC_IN_HR / MSEC_IN_MIN;
  (*sec) = value % MSEC_IN_MIN / MSEC_IN_SEC;
}

// Get the timer time in milliseconds assuming the following conditions
// 1. when the timer is running, start_ms represents the epoch when it was started
// 2. when it is paused, start_ms represents the negative of the time is has been running
int64_t timer_get_value_ms(void) {
  int64_t value = timer_data.length_ms - epoch() +
    (((timer_data.start_ms + epoch() - 1) % epoch()) + 1);
  if (value < 0) {
    return -value;
  }
  return value;
}

// Get the total timer time in milliseconds
int64_t timer_get_length_ms(void) {
  return timer_data.length_ms;
}

// Check if the timer is vibrating
bool timer_is_vibrating(void) {
  return timer_is_chrono() && !timer_is_paused() && timer_data.can_vibrate;
}

// Check if timer is in stopwatch mode
bool timer_is_chrono(void) {
  // see timer_get_timer_parts for explanation of equation
  return timer_data.length_ms - (int64_t)epoch() +
    ((timer_data.start_ms + (int64_t)epoch() - 1) % (int64_t)epoch() + 1) <= 0;
}

// Check if timer or stopwatch is paused
bool timer_is_paused(void) {
  return timer_data.start_ms <= 0;
}

// Check if the timer is elapsed and vibrate if this is the first call after elapsing
void timer_check_elapsed(void) {
  if (timer_is_chrono() && !timer_is_paused() && timer_data.can_vibrate) {
    // stop vibration after certain duration
    if (timer_get_value_ms() > VIBRATION_LENGTH_MS) {
      timer_data.can_vibrate = false;
    }
    // vibrate
    vibes_enqueue_custom_pattern(vibe_pattern);
  }
}

// Increment timer value currently being edited
void timer_increment(int64_t increment) {
  // if in paused stopwatch mode, rewind to previous time
  if (timer_is_chrono() && timer_data.start_ms) {
    timer_rewind();
    return;
  }
  // identify increment class
  int64_t interval;
  if (abs(increment) < MSEC_IN_MIN) {
    interval = MSEC_IN_MIN;
  } else if (abs(increment) < MSEC_IN_HR) {
    interval = MSEC_IN_HR;
  } else {
    interval = MSEC_IN_HR * 100;
  }
  // calculate new time by incrementing with wrapping
  int64_t ls_bit = (timer_data.length_ms + timer_data.start_ms) % interval;
  int64_t step = (ls_bit + interval + increment) % interval - ls_bit;
  if (timer_data.start_ms) {
    timer_data.start_ms += step;
    if (timer_data.start_ms > 0) {
      timer_data.length_ms += timer_data.start_ms;
      timer_data.start_ms = 0;
    }
  } else {
    timer_data.length_ms += step;
  }
  // if at zero, remove any leftover milliseconds
  if (timer_get_value_ms() < MSEC_IN_SEC) {
    timer_reset();
  }
  // enable vibration
  if (timer_data.length_ms) {
    timer_data.can_vibrate = true;
  }
}

// Toggle play pause state for timer
void timer_toggle_play_pause(void) {
  if (timer_data.start_ms > 0) {
    timer_data.start_ms -= epoch();
  } else {
    timer_data.start_ms += epoch();
  }
}

//! Rewind the timer back to its original value
void timer_rewind(void) {
  timer_data.start_ms = 0;
  // enable vibration
  if (timer_data.length_ms) {
    timer_data.can_vibrate = true;
  }
}

// Reset the timer to zero
void timer_reset(void) {
  timer_data.length_ms = 0;
  timer_data.start_ms = 0;
  // disable vibration
  timer_data.can_vibrate = false;
}

// Save the timer to persistent storage
void timer_persist_store(void) {
  // write out current persistent data version for potential future reference
  persist_write_int(PERSIST_VERSION_KEY, PERSIST_VERSION);
  persist_write_data(PERSIST_TIMER_KEY, &timer_data, sizeof(timer_data));
}

// Read the timer from persistent storage
void timer_persist_read(void) {
  // read legacy version
  if (persist_exists(PERSIST_TIMER_KEY_V2)) {
    persist_delete(PERSIST_TIMER_KEY_V2);
    if (launch_reason() == APP_LAUNCH_WAKEUP) {
      timer_increment(5000);
      timer_toggle_play_pause();
    }
  }
  // read current version
  if (persist_exists(PERSIST_TIMER_KEY)) {
    persist_read_data(PERSIST_TIMER_KEY, &timer_data, sizeof(timer_data));
  } else {
    timer_reset();
  }
}
