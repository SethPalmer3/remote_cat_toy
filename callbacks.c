#include "callbacks.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#define SIMPLE_CALLBACK "Hello\0"

// The C string for the HTTP response
const char *http_response =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: 5\r\n" // Length of the body "hello"
    "Connection: close\r\n" // Tell client to close connection after this
                            // response
    "\r\n"                  // End of headers
    "hello";                // The body

// Calculate the total length of this string for sending
// strlen(http_response) would give you this.

static void show_characters(char *s, int len) {
  for (int i = 0; i < len; i++) {
    printf("%c", s[i]);
  }
  printf("\n");
}

// Callback function for receiving data
err_t tcp_server_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p,
                               err_t err) {
  LWIP_UNUSED_ARG(arg);

  if (p == NULL) {
    printf("client disconnected or error in recv.\n");
    return tcp_close(tpcb);
  }

  if (err == ERR_OK) {
    tcp_recved(tpcb, p->tot_len);
    printf("received %u bytes: ", p->tot_len);
    show_characters(p->payload, p->tot_len);
    // Responding
    if (tcp_write(tpcb, http_response, strlen(http_response),
                  TCP_WRITE_FLAG_COPY)) {
      printf("Failed to respond\n");
    } else {
      tcp_output(tpcb);
    }
    pbuf_free(p);
    return tcp_close(tpcb);
  } else {
    printf("recieve error: %d\n", err);
    pbuf_free(p);
    return ERR_OK;
  }
}

// Callback function for new connections
err_t tcp_server_accept_callback(void *arg, struct tcp_pcb *new_pcb,
                                 err_t err) {
  LWIP_UNUSED_ARG(arg);

  if (err != ERR_OK || new_pcb == NULL) {
    printf("accept callback error: %d\n", err);
    return ERR_VAL;
  }
  printf("client connected!\n");

  tcp_accepted(new_pcb);

  tcp_recv(new_pcb, tcp_server_recv_callback);

  return ERR_OK;
}
