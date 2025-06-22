#include "pico/cyw43_arch.h"
#include <stdlib.h>

// --- Handler ---

// Left and right are in reference with the usb facing toward the person
typedef struct mv_hndlr {
  uint right_forward;
  uint right_backward;
  uint left_forward;
  uint left_backward;
  void (*turn_left)(struct mv_hndlr *);
  void (*turn_right)(struct mv_hndlr *);
  void (*move_forward)(struct mv_hndlr *);
  void (*move_backward)(struct mv_hndlr *);
} movementHandler;

// --- Movement Functions ---

/*
 * @breif int direction > 0 for forward, 0 if no movement, < 0 for backward
 * @brief forward_pin the pin to set high to go forward
 * @brief reverse_pin the pin to set high to go backward
 */
void move(int direction, uint forward_pin, uint reverse_pin);
