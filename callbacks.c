#include "callbacks.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SIMPLE_CALLBACK "Hello\0"
#define GEN_ACTION(x) "GET /button?action=" #x

char http_headers[512];
// The C string for the HTTP response
char *controller_html =
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<title>Directional Buttons</title>"
    "<script>"
    "let actionIntervalId = null;"
    "const actionRate = 200;"
    "function sendButtonPress(direction) {"
    "fetch('/button?action=' + direction)"
    ".then(response => {"
    "if (!response.ok) {"
    "console.error('Action request failed with status: ' + response.status);"
    "}"
    "})"
    ".catch(error => {"
    "console.error('Error sending action request:', error);"
    "});"
    "}"
    ""
    "function startAction(direction) {"
    "if (actionIntervalId !== null) {"
    "return;"
    "}"
    "sendButtonPress(direction);"
    "console.log('Action started: ' + direction);"
    "actionIntervalId = setInterval(() => {"
    "sendButtonPress(direction);"
    "}, actionRate);"
    "}"
    "function stopAction() {"
    "if (actionIntervalId === null) {"
    "return;"
    "}"
    "clearInterval(actionIntervalId);"
    "actionIntervalId = null;"
    "console.log('Action stopped.');"
    "}"
    "</script>"
    "</head>"
    "<body>"
    "<center>"
    "<p>"
    "<button type=\"button\" id=\"buttonUp\" "
    "onmousedown=\"startAction(\'up\')\" "
    "onmouseup=\"stopAction()\" "
    "onmouseleave=\"stopAction()\">Up</button>"
    "</p>"
    "<p>"
    "<button type=\"button\" id=\"buttonLeft\" style=\"margin-right: 40px;\" "
    "onmousedown=\"startAction(\'left\')\" "
    "onmouseup=\"stopAction()\" "
    "onmouseleave=\"stopAction()\">Left</button>"
    "<button type=\"button\" id=\"buttonRight\" style=\"margin-left: 40px;\" "
    "onmousedown=\"startAction(\'right\')\" "
    "onmouseup=\"stopAction()\" "
    "onmouseleave=\"stopAction()\">Right</button>"
    "</p>"
    "<p>"
    "<button type=\"button\" id=\"buttonDown\" "
    "onmousedown=\"startAction(\'down\')\" "
    "onmouseup=\"stopAction()\" "
    "onmouseleave=\"stopAction()\">Down</button>"
    "</p>"
    "</center>"
    "</body>"
    "</html>";
// Calculate the total length of this string for sending
// strlen(http_response) would give you this.

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
    printf(" -- received %u bytes: \n", p->tot_len);

    printf("%.*s\n", p->tot_len, p->payload);
    printf("testing [%.*s] against [%s]\n\n", p->payload,
           strlen(GEN_ACTION(up)), GEN_ACTION(up));
    // Checking if the request was a action
    if (strncmp(p->payload, GEN_ACTION(up), strlen(GEN_ACTION(up))) == 0) {
      // Move forward
      printf("[Action]Moving forward\n");
    } else if (strncmp(p->payload, GEN_ACTION(down),
                       strlen(GEN_ACTION(down))) == 0) {
      // Move forward
      printf("[Action]Moving back\n");
    } else if (strncmp(p->payload, GEN_ACTION(left),
                       strlen(GEN_ACTION(left))) == 0) {
      // Move forward
      printf("[Action]Moving left\n");
    } else if (strncmp(p->payload, GEN_ACTION(right),
                       strlen(GEN_ACTION(right))) == 0) {
      // Move forward
      printf("[Action]Moving right\n");
    }
    // Responding
    int body_len = strlen(controller_html);
    printf("body length: %d\n", body_len);
    // Dynamically adding the length of the body
    (void)sprintf(http_headers,
                  "HTTP/1.1 200 OK\r\n"
                  "Content-Type: text/html\r\n"
                  "Content-Length: %d\r\n" // Length of the body
                  "Connection: close\r\n"  // Tell client to close connection
                  "\r\n",                  // End of headers
                  body_len);
    char *http_response =
        (char *)malloc(sizeof(char) * (strlen(http_headers) + body_len));
    printf("length of http response: %d\n",
           (int)strlen(http_headers) + body_len);
    strcpy(http_response, http_headers);
    strcpy(http_response + strlen(http_headers), controller_html);
    if (tcp_write(tpcb, http_response, strlen(http_response),
                  TCP_WRITE_FLAG_COPY)) {
      printf("Failed to respond\n");
    } else {
      tcp_output(tpcb);
    }
    free(http_response);
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
