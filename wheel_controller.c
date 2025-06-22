#include "wheel_controller.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"
#include <stdlib.h>

void move(int direction, uint forward_pin, uint reverse_pin) {
  gpio_init(forward_pin);
  gpio_init(reverse_pin);
  gpio_set_dir(forward_pin, GPIO_OUT);
  gpio_set_dir(reverse_pin, GPIO_OUT);
  if (direction > 0) { // Forward_direction
    gpio_put(forward_pin, 1);
    gpio_put(reverse_pin, 0);
  } else if (direction == 0) {
    gpio_put(forward_pin, 0);
    gpio_put(reverse_pin, 0);
  } else if (direction < 0) {
    gpio_put(forward_pin, 0);
    gpio_put(reverse_pin, 1);
  }
}
