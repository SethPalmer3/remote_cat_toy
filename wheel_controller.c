#include "wheel_controller.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"

void wheel_init(uint forward_pin, uint backward_pin) {
  gpio_init(forward_pin);
  gpio_init(backward_pin);
  gpio_set_dir(forward_pin, GPIO_OUT);
  gpio_set_dir(backward_pin, GPIO_OUT);
}

void move(int direction, uint forward_pin, uint reverse_pin) {

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
