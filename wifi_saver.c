#include "wifi_saver.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

// --- AP Mode Configuration ---
#define AP_SSID "PicoW_AP"
#define AP_PASSWORD "PicoW_AP"
#define AP_CHANNEL 11
#define AP_AUTH CYW43_AUTH_WPA2_AES_PSK

// --- Static IP Configuration for the Pico W ---
#define AP_IP "192.168.4.1"
#define AP_NETMASK "255.255.255.0"
#define AP_GATEWAY "192.168.4.1" // Often the same as the AP's IP

#include <stdio.h>

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
  //
  // // --- Initialize and Start the DHCP Server ---
  // printf("Starting DHCP server...\n");
  dhcpserver_init(&dhcp_server);
  // printf("DHCP server started.\n");
  //
  // printf("Pico W is now an Access Point. Connect to it!\n");
  //
  // // The main loop can be used for other tasks.
  // // LwIP and the DHCP server run in the background.
  while (1) {
    // Your application logic can go here.
    // For example, toggle the onboard LED.
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_ms(500);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    sleep_ms(500);
  }
}
