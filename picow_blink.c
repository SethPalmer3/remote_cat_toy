#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdio.h>

int main() {
  stdio_init_all();

  const uint PIN_2 = 2;
  const uint PIN_3 = 3;
  const uint PIN_4 = 4;
  const uint PIN_5 = 5;

  gpio_init(PIN_2);
  gpio_set_dir(PIN_2, GPIO_OUT);
  gpio_init(PIN_3);
  gpio_set_dir(PIN_3, GPIO_OUT);
  gpio_init(PIN_4);
  gpio_set_dir(PIN_4, GPIO_OUT);
  gpio_init(PIN_5);
  gpio_set_dir(PIN_5, GPIO_OUT);

  while (true) {
    printf("Setting pins 2 and 4 HIGH, 3 and 5 LOW\n");
    gpio_put(PIN_2, 1);
    gpio_put(PIN_3, 0);
    gpio_put(PIN_4, 1);
    gpio_put(PIN_5, 0);
    sleep_ms(2000);

    printf("Setting pins 2 and 4 LOW, 3 and 5 HIGH\n");
    gpio_put(PIN_2, 0);
    gpio_put(PIN_3, 1);
    gpio_put(PIN_4, 0);
    gpio_put(PIN_5, 1);
    sleep_ms(2000);
  }
}
