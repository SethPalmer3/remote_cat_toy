#include <stdio.h>

#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#define WIFI_SSID "MOTOA060"
#define WIFI_PASS "808n7ne2u3"

int main(int argc, char *argv[]) {
  stdio_init_all();

  if (cyw43_arch_init_with_country(CYW43_COUNTRY_UK)) {
    printf("failed to initalise\n");
    return 1;
  }

  cyw43_arch_enable_sta_mode();

  if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS,
                                         CYW43_AUTH_WPA2_AES_PSK, 10000)) {
    printf("failed to connect\n");
    return 1;
  }
  printf("connect\n");
  return 0;
}
