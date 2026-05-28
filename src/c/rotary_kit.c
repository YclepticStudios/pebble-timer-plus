// rotary_kit.c — RotaryKit implementation
// See rotary_kit.h for documentation.

#include "rotary_kit.h"

// ---------------------------------------------------------------------------
// Internal state (all private to this translation unit)
// ---------------------------------------------------------------------------

// Lookup table: atan in half-degrees for the first octant.
// Index i maps (opposite/adjacent * 45) → half-degree angle [0..90].
static const int8_t s_atan_table[46] = {
     0,  3,  5,  8, 10, 13, 15, 18, 20, 23,
    25, 27, 30, 32, 35, 37, 39, 41, 44, 46,
    48, 50, 52, 54, 56, 58, 60, 62, 64, 66,
    67, 69, 71, 73, 74, 76, 77, 79, 80, 82,
    83, 85, 86, 87, 89, 90
};

static bool s_active = false;

// ---------------------------------------------------------------------------
// Rotation acceleration
//
// Configurable via RotaryConfig: accel_degrees_per_level, accel_max_level,
// accel_reset_ms.  Set accel_degrees_per_level = 0 to disable.
// ---------------------------------------------------------------------------

static int32_t   s_accel_total_hd   = 0;
static int       s_accel_multiplier = 1;
static AppTimer *s_accel_timer      = NULL;

static void prv_accel_reset(void *data) {
    s_accel_timer      = NULL;
    s_accel_total_hd   = 0;
    s_accel_multiplier = 1;
}

// ---------------------------------------------------------------------------
// Per-window config table
//
// Stores a (Window *, RotaryConfig) pair for each registered window.
// On Touchdown, the top window is looked up and its config drives the gesture
// until Liftoff — so pushing/popping windows mid-app just works.
// ---------------------------------------------------------------------------

#define MAX_WINDOW_CONFIGS 8

typedef struct {
    Window      *window;
    RotaryConfig config;
} WindowConfigEntry;

static WindowConfigEntry s_window_configs[MAX_WINDOW_CONFIGS];
static int               s_window_config_count = 0;

// Returns the config for the given window, or NULL if not registered.
static RotaryConfig *prv_find_window_config(Window *window) {
    for (int i = 0; i < s_window_config_count; i++) {
        if (s_window_configs[i].window == window) {
            return &s_window_configs[i].config;
        }
    }
    return NULL;
}

// ---------------------------------------------------------------------------
// Swipe region
//
// The screen is divided into four 90° wedges measured clockwise from 12-o'clock:
//
//   UP    region:  315° – 45°   (top)
//   RIGHT region:   45° – 135°  (right)
//   DOWN  region:  135° – 225°  (bottom)
//   LEFT  region:  225° – 315°  (left)
//
// REGION_NONE means the touch is inside the dead zone.
// ---------------------------------------------------------------------------

typedef enum {
    REGION_NONE  = -1,
    REGION_UP    =  0,   // values map 1:1 to RotarySwipeDirection_*
    REGION_DOWN  =  1,
    REGION_LEFT  =  2,
    REGION_RIGHT =  3,
} TouchRegion;

// Opposite region table — used to validate a cross-screen swipe.
static const TouchRegion OPPOSITE[4] = {
    REGION_DOWN,   // opposite of UP
    REGION_UP,     // opposite of DOWN
    REGION_RIGHT,  // opposite of LEFT
    REGION_LEFT,   // opposite of RIGHT
};

// ---------------------------------------------------------------------------
// Active gesture state (valid from Touchdown through Liftoff)
// ---------------------------------------------------------------------------

static RotaryConfig s_cfg;               // snapshot of the top window's config
static bool         s_cfg_valid = false; // false if no config found at Touchdown

static int16_t     s_last_angle_hd    = 0;
static int32_t     s_accumulated_hd   = 0;
static int32_t     s_total_hd         = 0;
static int         s_click_count      = 0;
static bool        s_is_rotating      = false;

static TouchRegion s_start_region     = REGION_NONE;
static TouchRegion s_last_region      = REGION_NONE;  // updated every PositionUpdate
static bool        s_centre_crossed   = false;
static bool        s_center_tap_pending = false;

// ---------------------------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------------------------

// Returns true if (x,y) is OUTSIDE the dead-zone (i.e. on the wheel ring).
static bool prv_on_wheel(int16_t x, int16_t y) {
    int32_t dx = x - s_cfg.center_x;
    int32_t dy = y - s_cfg.center_y;
    int32_t r  = s_cfg.min_radius;
    return (dx * dx + dy * dy) >= r * r;
}

// Returns the touch angle in half-degrees [0..720), 0 = right, CW positive.
static int16_t prv_coords_to_angle_hd(int16_t x, int16_t y) {
    int16_t dx  = x - s_cfg.center_x;
    int16_t dy  = y - s_cfg.center_y;

    if (dx == 0 && dy == 0) return 0;

    int16_t adx = dx < 0 ? -dx : dx;
    int16_t ady = dy < 0 ? -dy : dy;
    int8_t  oct_hd;

    if (adx >= ady) {
        oct_hd = s_atan_table[(ady * 45) / adx];
        if      (dx >= 0 && dy >= 0) return oct_hd;
        else if (dx <  0 && dy >= 0) return 360 - oct_hd;
        else if (dx <  0 && dy <  0) return 360 + oct_hd;
        else                         return 720 - oct_hd;
    } else {
        oct_hd = s_atan_table[(adx * 45) / ady];
        if      (dx >= 0 && dy >= 0) return 180 - oct_hd;
        else if (dx <  0 && dy >= 0) return 180 + oct_hd;
        else if (dx <  0 && dy <  0) return 540 - oct_hd;
        else                         return 540 + oct_hd;
    }
}

// Shortest signed delta between two half-degree angles, noise-clamped.
// Jumps > 30° (= 60 hd) in a single frame are discarded as sensor noise.
static int16_t prv_angle_delta_hd(int16_t from, int16_t to) {
    int16_t delta = to - from;
    if (delta >  360) delta -= 720;
    if (delta < -360) delta += 720;
    if (delta >   60) return 0;
    if (delta <  -60) return 0;
    return delta;
}

// Returns which swipe region contains (x,y), or REGION_NONE if in dead zone.
// prv_coords_to_angle_hd returns 0=right, CW. Add 180 hd (90°) to rotate
// to 0=top, CW, then bucket into 90° wedges.
//   Verified cardinal points:
//     right  (dx>0, dy=0) → raw=0,   top=180, deg=90  → REGION_RIGHT ✓
//     bottom (dx=0, dy>0) → raw=180, top=360, deg=180 → REGION_DOWN  ✓
//     left   (dx<0, dy=0) → raw=360, top=540, deg=270 → REGION_LEFT  ✓
//     top    (dx=0, dy<0) → raw=540, top=0,   deg=0   → REGION_UP    ✓
static TouchRegion prv_region_at(int16_t x, int16_t y) {
    if (!prv_on_wheel(x, y)) return REGION_NONE;

    int16_t raw_hd = prv_coords_to_angle_hd(x, y);
    int16_t top_hd = (raw_hd + 180) % 720;  // rotate so 0 = top, CW
    int16_t deg    = top_hd / 2;             // 0–359°

    if (deg < 45 || deg >= 315) return REGION_UP;
    if (deg < 135)              return REGION_RIGHT;
    if (deg < 225)              return REGION_DOWN;
    return REGION_LEFT;
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Fires a single-segment custom vibe pattern of `ms` milliseconds.
// Does nothing if ms == 0. Using vibes_enqueue_custom_pattern instead of
// vibes_short_pulse gives precise control over intensity/duration.
static void prv_vibe(uint32_t ms) {
    if (ms == 0) return;
    VibePattern pat = {
        .durations    = &ms,
        .num_segments = 1,
    };
    vibes_enqueue_custom_pattern(pat);
}

static void prv_fire_click(int direction) {
    s_click_count++;
    prv_vibe(s_cfg.click_vibe_ms);
    if (s_cfg.on_click) {
        s_cfg.on_click(direction, s_click_count, s_cfg.context);
    }
    if (s_cfg.accel_reset_ms > 0) {
        if (s_accel_timer) {
            app_timer_reschedule(s_accel_timer, s_cfg.accel_reset_ms);
        } else {
            s_accel_timer = app_timer_register(s_cfg.accel_reset_ms, prv_accel_reset, NULL);
        }
    }
}

// ---------------------------------------------------------------------------
// Touch service handler
// ---------------------------------------------------------------------------

static void prv_touch_handler(const TouchEvent *event, void *context) {

    switch (event->type) {

        case TouchEvent_Touchdown:
            // Look up the top window's config — this snapshot drives the whole gesture.
            {
                Window *top = window_stack_get_top_window();
                RotaryConfig *cfg = top ? prv_find_window_config(top) : NULL;
                if (cfg) {
                    s_cfg       = *cfg;
                    s_cfg_valid = true;
                } else {
                    s_cfg_valid = false;
                }
            }

            // Reset all per-gesture state.
            s_click_count        = 0;
            s_accumulated_hd     = 0;
            s_total_hd           = 0;
            s_is_rotating        = false;
            s_centre_crossed     = false;
            s_center_tap_pending = false;
            s_start_region       = s_cfg_valid
                                       ? prv_region_at(event->x, event->y)
                                       : REGION_NONE;
            s_last_region        = s_start_region;

            if (s_start_region != REGION_NONE) {
                s_is_rotating   = true;
                s_last_angle_hd = prv_coords_to_angle_hd(event->x, event->y);
            } else if (s_cfg_valid) {
                // In dead zone — could be a centre tap.
                s_center_tap_pending = true;
            }
            break;

        case TouchEvent_PositionUpdate:
            if (!s_cfg_valid) break;

            // Track centre crossing for swipe detection.
            if (!s_centre_crossed && !prv_on_wheel(event->x, event->y)) {
                s_centre_crossed = true;
            }

            // Always track the last region the finger was seen on the wheel.
            if (prv_on_wheel(event->x, event->y)) {
                s_last_region = prv_region_at(event->x, event->y);
            }

            // If the finger started in the dead zone and has moved onto the wheel,
            // cancel the centre tap and begin rotation tracking.
            if (s_center_tap_pending && prv_on_wheel(event->x, event->y)) {
                s_center_tap_pending = false;
                s_is_rotating        = true;
                s_start_region       = s_last_region;
                s_last_angle_hd      = prv_coords_to_angle_hd(event->x, event->y);
            }

            // Accumulate rotation while the finger is on the wheel.
            if (s_is_rotating && prv_on_wheel(event->x, event->y)) {
                int16_t cur_hd = prv_coords_to_angle_hd(event->x, event->y);
                int16_t delta  = prv_angle_delta_hd(s_last_angle_hd, cur_hd);
                s_accumulated_hd += delta;
                s_total_hd       += delta < 0 ? -delta : delta;
                s_last_angle_hd   = cur_hd;

                // Update the acceleration multiplier if enabled.
                if (s_cfg.accel_degrees_per_level > 0) {
                    int16_t abs_delta = delta < 0 ? -delta : delta;
                    s_accel_total_hd += abs_delta;
                    int level = (int)(s_accel_total_hd / (s_cfg.accel_degrees_per_level * 2));
                    if (level > s_cfg.accel_max_level) level = s_cfg.accel_max_level;
                    s_accel_multiplier = 1 << level;
                }

                // Threshold shrinks as multiplier grows → more clicks per arc.
                int16_t threshold_hd = (s_cfg.degrees_per_click * 2) / s_accel_multiplier;
                if (threshold_hd < 1) threshold_hd = 1;

                while (s_accumulated_hd >= threshold_hd) {
                    prv_fire_click(+1);
                    s_accumulated_hd -= threshold_hd;
                }
                while (s_accumulated_hd <= -threshold_hd) {
                    prv_fire_click(-1);
                    s_accumulated_hd += threshold_hd;
                }
            }
            break;

        case TouchEvent_Liftoff: {
            if (!s_cfg_valid) break;

            // Centre tap: finger stayed in the dead zone the whole time.
            if (s_center_tap_pending) {
                s_center_tap_pending = false;
                if (s_cfg.on_center_tap) {
                    s_cfg.on_center_tap(s_cfg.context);
                }
                break;
            }

            // Swipe: start region + centre crossed + last seen region is the opposite.
            // We use s_last_region (last PositionUpdate on the wheel) rather than
            // the liftoff event coordinates, which are unreliable on Pebble Time Round.
            if (s_centre_crossed          &&
                s_start_region != REGION_NONE &&
                s_last_region  != REGION_NONE &&
                s_last_region == OPPOSITE[s_start_region]) {

                // Direction = the side the finger moved toward.
                RotarySwipeDirection dir = (RotarySwipeDirection)s_last_region;

                prv_vibe(s_cfg.swipe_vibe_ms);
                if (s_cfg.on_swipe) {
                    s_cfg.on_swipe(dir, s_cfg.context);
                }
            } else {
                // Not a valid swipe — report as rotation liftoff.
                if (s_cfg.on_liftoff) {
                    s_cfg.on_liftoff(s_click_count,
                                     (int)s_total_hd / 2,
                                     s_cfg.context);
                }
            }

            // Reset gesture state.
            s_is_rotating    = false;
            s_accumulated_hd = 0;
            s_centre_crossed = false;
            s_start_region   = REGION_NONE;
            s_last_region    = REGION_NONE;
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

RotaryConfig rotary_kit_default_config(void) {
    RotaryConfig cfg = {
        .center_x          = 130,
        .center_y          = 130,
        .min_radius        = 40,
        .degrees_per_click = 30,
        .click_vibe_ms     = 20,   // subtle — won't fatigue during fast scrolling
        .swipe_vibe_ms     = 40,   // slightly longer — marks a committed gesture
        .accel_degrees_per_level = 180,
        .accel_max_level         = 3,
        .accel_reset_ms          = 500,
        .on_click          = NULL,
        .on_liftoff        = NULL,
        .on_center_tap     = NULL,
        .on_swipe          = NULL,
        .context           = NULL,
    };
    return cfg;
}

void rotary_kit_set_window_config(Window *window, const RotaryConfig *config) {
    // Update if already registered.
    for (int i = 0; i < s_window_config_count; i++) {
        if (s_window_configs[i].window == window) {
            s_window_configs[i].config = *config;
            return;
        }
    }

    // New registration.
    if (s_window_config_count >= MAX_WINDOW_CONFIGS) {
        APP_LOG(APP_LOG_LEVEL_ERROR,
                "RotaryKit: MAX_WINDOW_CONFIGS (%d) reached — ignoring window",
                MAX_WINDOW_CONFIGS);
        return;
    }

    s_window_configs[s_window_config_count].window = window;
    s_window_configs[s_window_config_count].config = *config;
    s_window_config_count++;

    // Subscribe to the touch service on the first registration.
    if (!s_active) {
        if (touch_service_is_enabled()) {
            touch_service_subscribe(prv_touch_handler, NULL);
            s_active = true;
            APP_LOG(APP_LOG_LEVEL_INFO, "RotaryKit: touch service subscribed");
        } else {
            APP_LOG(APP_LOG_LEVEL_WARNING,
                    "RotaryKit: touch service unavailable on this hardware");
        }
    }
}

void rotary_kit_clear_window_config(Window *window) {
    for (int i = 0; i < s_window_config_count; i++) {
        if (s_window_configs[i].window == window) {
            // Shift remaining entries down.
            for (int j = i; j < s_window_config_count - 1; j++) {
                s_window_configs[j] = s_window_configs[j + 1];
            }
            s_window_config_count--;
            break;
        }
    }

    // Unsubscribe when the last window is removed.
    if (s_window_config_count == 0 && s_active) {
        touch_service_unsubscribe();
        s_active             = false;
        s_is_rotating        = false;
        s_centre_crossed     = false;
        s_center_tap_pending = false;
        s_start_region       = REGION_NONE;
        s_last_region        = REGION_NONE;
        if (s_accel_timer) {
            app_timer_cancel(s_accel_timer);
            s_accel_timer      = NULL;
        }
        s_accel_total_hd   = 0;
        s_accel_multiplier = 1;
        APP_LOG(APP_LOG_LEVEL_INFO, "RotaryKit: touch service unsubscribed");
    }
}

bool rotary_kit_is_active(void) {
    return s_active;
}
