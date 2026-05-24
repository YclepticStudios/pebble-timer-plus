// rotary_kit.h — RotaryKit: iPod-style click wheel gesture library for Pebble
// Drop rotary_kit.h + rotary_kit.c into your project and include this header.
//
// Usage:
//   1. Call rotary_kit_set_window_config() when creating each window.
//   2. Call rotary_kit_clear_window_config() when destroying it.
//   That's it — RotaryKit owns the touch subscription and automatically routes
//   events to whichever window is on top of the stack.

#pragma once
#include <pebble.h>

// ---------------------------------------------------------------------------
// Callback types
// ---------------------------------------------------------------------------

// Fired every time the wheel rotates enough for one "detent" click.
//   direction: +1 = clockwise (scroll down), -1 = counter-clockwise (scroll up)
//   click_num: 1-based count of clicks fired in the current drag gesture
typedef void (*RotaryClickCallback)(int direction, int click_num, void *context);

// Fired when the user lifts their finger after a rotation (optional).
//   total_clicks : total click-events fired during this gesture
//   total_degrees: total absolute rotation in degrees (always positive)
typedef void (*RotaryLiftoffCallback)(int total_clicks, int total_degrees, void *context);

// Fired when the user taps and releases inside the dead-zone centre without
// drifting onto the wheel — equivalent to pressing the physical Select button.
// (optional — pass NULL to skip)
typedef void (*RotaryCenterTapCallback)(void *context);

// Direction reported by on_swipe.
typedef enum {
    RotarySwipeDirection_Up    = 0,
    RotarySwipeDirection_Down  = 1,
    RotarySwipeDirection_Left  = 2,
    RotarySwipeDirection_Right = 3,
} RotarySwipeDirection;

// Fired when a cross-screen swipe is recognised (on liftoff).
//   direction: one of the four RotarySwipeDirection values.
// (optional — pass NULL to skip)
typedef void (*RotarySwipeCallback)(RotarySwipeDirection direction, void *context);

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

typedef struct {
    // --- Geometry ---
    int16_t center_x;           // X pixel of wheel centre (default: 130)
    int16_t center_y;           // Y pixel of wheel centre (default: 130)
    int16_t min_radius;         // Dead zone radius in px — touches inside are
                                //   treated as centre taps (default: 40)

    // --- Sensitivity ---
    int16_t degrees_per_click;  // Degrees of arc per rotation detent (default: 30)
                                //   Lower = more sensitive, higher = coarser

    // --- Haptics ---
    // Duration in milliseconds for the vibration pulse on each event.
    // Set to 0 to disable vibration entirely for that event.
    // Default: 20ms for clicks (subtle), 40ms for swipes (slightly more distinct).
    // Maximum: 10000ms (Pebble VibePattern limit).
    uint32_t click_vibe_ms;   // pulse duration per rotation detent  (default: 20)
    uint32_t swipe_vibe_ms;   // pulse duration on swipe fire        (default: 40)

    // --- Callbacks ---
    RotaryClickCallback     on_click;       // Required
    RotaryLiftoffCallback   on_liftoff;     // Optional — pass NULL
    RotaryCenterTapCallback on_center_tap;  // Optional — pass NULL
    RotarySwipeCallback     on_swipe;       // Optional — pass NULL

    // --- User data passed back to all callbacks ---
    void *context;
} RotaryConfig;

// ---------------------------------------------------------------------------
// API
// ---------------------------------------------------------------------------

// Returns a RotaryConfig pre-filled with sensible defaults.
// Override only the fields you care about before passing to rotary_kit_set_window_config().
RotaryConfig rotary_kit_default_config(void);

// Register a gesture config for a specific window.
// Call from your window's .load handler (or before pushing it onto the stack).
// RotaryKit subscribes to the touch service automatically on the first registration.
// Up to MAX_WINDOW_CONFIGS (8) windows may be registered simultaneously.
void rotary_kit_set_window_config(Window *window, const RotaryConfig *config);

// Remove the gesture config for a window.
// Call from your window's .unload handler.
// RotaryKit unsubscribes from the touch service automatically when the last
// window is removed.
void rotary_kit_clear_window_config(Window *window);

// Returns true if RotaryKit is currently subscribed to the touch service.
bool rotary_kit_is_active(void);