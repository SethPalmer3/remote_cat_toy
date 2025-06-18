#include "hardware/flash.h"
#include "hardware/sync.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "callbacks.h"
#include "storage.h"
#include "wifi_saver.h"
// #include "storage.h"

#define TCP_SERVER_PORT 80

#define MAX_TRIES 5

int provision_mode = 0;

int main(void) {
  stdio_init_all();
  printf("%d", FLASH_SECTOR_SIZE);
  if (cyw43_arch_init_with_country(CYW43_COUNTRY_USA)) {
    printf("failed to initalise\n");
    return 1;
  }
  printf("initalised\n");
  cyw43_arch_enable_sta_mode();
  printf("station mode enabled\n");

  int tries = 0;
  char ssid[SSID_MAX_LEN];
  char pass[MAX_PASS_LEN];
  memcpy(ssid, read_ssid(), SSID_MAX_LEN);
  memcpy(pass, read_password(), MAX_PASS_LEN);
  printf("Trying to connect to %s, with %s\n", ssid, pass);
  while (cyw43_arch_wifi_connect_timeout_ms(ssid, pass, CYW43_AUTH_WPA2_AES_PSK,
                                            10000) &&
         tries < MAX_TRIES) {
    printf("failed to connect, retrying\n");
    tries++;
  }
  if (tries >= MAX_TRIES) {
    printf("Starting own AP\n");
    provision_mode = 1;
    collect_wifi();
  } else {
    provision_mode = 0;
  }
  static dhcpserver_t dhcp_server;
  // ...
  dhcpserver_init(&dhcp_server);
  printf("DHCP server started.\n");

  // --- ALSO START THE TCP SERVER HERE ---
  printf("Starting configuration TCP server...\n");
  struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
  if (!pcb) {
    return;
  }                                           // Handle error
  err_t err = tcp_bind(pcb, IP_ANY_TYPE, 80); // Bind to port 80 for HTTP
  if (err != ERR_OK) {
    tcp_close(pcb);
    return;
  }
  struct tcp_pcb *listening_pcb = tcp_listen(pcb);
  if (!listening_pcb) {
    tcp_close(pcb);
    return;
  }
  tcp_accept(listening_pcb, tcp_server_accept_callback);

  printf("Pico W is in configuration mode. Connect to the '%s' network.\n",
         AP_SSID);
  printf("connected\n");
  struct tcp_pcb *pcb;
  pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
  if (!pcb) {
    printf("failed to create tcp pcb\n");
    return EXIT_FAILURE;
  }
  printf("tcp pcb created\n");

  err_t err = tcp_bind(pcb, IP_ANY_TYPE, TCP_SERVER_PORT);
  if (err != ERR_OK) {
    printf("failed to bind tcp pcb, error: %d\n", err);
    tcp_close(pcb);
    return EXIT_FAILURE;
  }

  printf("tcp pcb bound to port %d\n", TCP_SERVER_PORT);

  struct tcp_pcb *listening_pcb = tcp_listen_with_backlog(pcb, 1);
  if (!listening_pcb) {
    printf("failed to listen on tcp pcb\n");
    if (pcb)
      tcp_close(pcb);
    return EXIT_FAILURE;
  }

  pcb = listening_pcb;
  printf("tcp server listening on port: %d\n", TCP_SERVER_PORT);

  tcp_arg(pcb, NULL);
  tcp_accept(pcb, tcp_server_accept_callback);
  printf("accept callback registered. server is ready\n");

  while (1) {
    sleep_ms(1000);
  }
  return EXIT_SUCCESS;
}
