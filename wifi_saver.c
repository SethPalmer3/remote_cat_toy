#include "wifi_saver.h"
#include "callbacks.h"
#include "dhcpserver.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include <stdio.h>

// --- AP Mode Configuration ---
#define AP_SSID "PicoW_AP"
#define AP_PASSWORD "PicoW_AP"
#define AP_CHANNEL 11
#define AP_AUTH CYW43_AUTH_WPA2_AES_PSK

// --- Static IP Configuration for the Pico W ---
#define AP_IP "192.168.4.1"
#define AP_NETMASK "255.255.255.0"
#define AP_GATEWAY "192.168.4.1" // Often the same as the AP's IP

void collect_wifi() {

  printf("Enabling AP mode...\n");
  cyw43_arch_disable_sta_mode();
  cyw43_arch_enable_ap_mode(AP_SSID, AP_PASSWORD, AP_AUTH);
  printf("AP mode enabled. SSID: %s\n", AP_SSID);

  // // --- Set up the Pico W's Static IP Address ---
  struct netif *netif = &cyw43_state.netif[CYW43_ITF_AP];
  ip4_addr_t ipaddr, netmask, gw;

  ip4addr_aton(AP_IP, &ipaddr);
  ip4addr_aton(AP_NETMASK, &netmask);
  ip4addr_aton(AP_GATEWAY, &gw);

  netif_set_addr(netif, &ipaddr, &netmask, &gw);
  printf("Pico W Static IP -> Addr: %s, Mask: %s, Gateway: %s\n", AP_IP,
         AP_NETMASK, AP_GATEWAY);
  static dhcp_server_t dhcp_server;
  dhcp_server_init(&dhcp_server, &ipaddr, &netmask);
  printf("DHCP server started.\n");

  // --- ALSO START THE TCP SERVER HERE ---
  printf("Starting configuration TCP server...\n");
  struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
  if (!pcb) {
    return;
  } // Handle error
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
  while (1) {
  }
}
