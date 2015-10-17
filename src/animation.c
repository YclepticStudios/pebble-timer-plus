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
#include "utility.h"

// Animation constants
#define ANIMATION_TICK_INTERVALE 30     //< Number of milliseconds to pause between animation ticks

// Animation pointer type
typedef struct AnimationNode {
  void (*step_func)(struct AnimationNode*);   //< Function to call when stepping animation
  void                    *target;            //< Pointer to value being animated
  struct AnimationNode    *next;              //< Pointer to next node in linked list
} AnimationNode;

// Animation framework data
static AnimationNode  *head_node = NULL;    //< Head node in linked list containing all animations
static AppTimer       *ani_timer = NULL;    //< AppTimer for stepping all animations


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Step a uint32 animation
static void prv_animation_step_uint32(AnimationNode *node) {
  // change target value
  (*(uint32_t*)node->target)++;
  // TODO: Develop this function to step animation target value and restart timer if not running
  // TODO: Also destroy timers which have finished
}

// Add node to end of linked list
static void prv_list_add_node(AnimationNode *node) {
  if (!head_node) {
    head_node = node;
    return;
  }
  AnimationNode *cur_node = head_node;
  while (cur_node->next) {
    cur_node = cur_node->next;
  }
  cur_node->next = node;
}

// Animation timer callback
static void prv_animation_timer_callback(void *data) {
  ani_timer = NULL;
  // loop over list and step each animation
  AnimationNode *cur_node = head_node;
  while (cur_node) {
    (*cur_node->step_func)(cur_node);
    cur_node = cur_node->next;
  }
}

// Start animation timer if not running
static void prv_animation_timer_start(void) {
  if (!ani_timer) {
    ani_timer = app_timer_register(ANIMATION_TICK_INTERVALE, &prv_animation_timer_callback, NULL);
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Functions
//

// Animate an integer by its pointer
void animation_uint32_start(uint32_t *ptr, uint32_t to, uint32_t duration) {
  // create and add new node
  AnimationNode *new_node = (AnimationNode*)MALLOC(sizeof(AnimationNode));
  new_node->step_func = &prv_animation_step_uint32;
  new_node->target = ptr;
  // TODO: Malloc from and to values
  new_node->next = NULL;
  prv_list_add_node(new_node);
  // start animation timer if not running
  prv_animation_timer_start();
}

// Animate a GRect by its pointer
void animation_grect_start(GRect *ptr, GRect to, uint32_t duration) {
  // TODO: Develop this function
}

// Cancel an animation by its pointer
void animation_stop(void *ptr) {
  // TODO: Find animation by target pointer and destroy animation
}
