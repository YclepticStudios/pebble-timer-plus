// @file animation.c
// @brief Animation framework to animate pointer values
//
// Animation framework to animate a pointer's value. Includes automatic
// detection of multiple animations per pointer, and destroys the oldest one.
// Animations also auto-destruct when complete
//
// @author Eric D. Phillips
// @date September 1, 2015
// @bugs No known bugs

#include "animation.h"

#include <pebble.h>


////////////////////////////////////////////////////////////////////////////////////////////////////
// Data
//

// Animation Constants
#define ANIMATION_COUNT_MAX 1

// Animation pointer type
typedef struct MyAnimation {
  Animation   *animation;   //< Pointer to animation
  void        *ptr;         //< Pointer to value being animated
} MyAnimation;

// Animation framework data singleton
static struct {
  MyAnimation   animations[ANIMATION_COUNT_MAX];    //< Array of all MyAnimations
  uint8_t       animation_count;                    //< Number of animations in animations array
} animation_data;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Find MyAnimation from pointer
MyAnimation *prv_find_my_animation_from_pointer(void *ptr) {

}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Functions
//

// Animate an integer by its pointer
void animation_uint32_start(uint32_t *ptr, uint32_t to, uint32_t duration) {

}

// Animate a GRect by its pointer
void animation_grect_start(GRect *ptr, GRect to, uint32_t duration) {

}

// Cancel an animation by its pointer
void animation_stop(void *ptr) {
  // loop and find MyAnimation from pointer

}

// Animation initialize singleton
void animation_initialize(void) {
  // zero values
  animation_data.animation_count = 0;
}
