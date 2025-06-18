#include "callbacks.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SIMPLE_CALLBACK "Hello\0"
#define GEN_ACTION(x) "GET /button?action=" #x

char http_headers[512];
// Wifi input html
char *WIFI_PROVISIONING_HTML =
    "<!DOCTYPE html>"
    "<html>"
    "<head><title>Pico W Wi-Fi Setup</title></head>"
    "<body>"
    "<h1>Wi-Fi Setup for Your Device</h1>"
    "<p>Please enter the credentials for your Wi-Fi network.</p>"
    "<form action=\"/save\" method=\"post\">"
    "<label for=\"ssid\">SSID:</label><br>"
    "<input type=\"text\" id=\"ssid\" name=\"ssid\"><br><br>"
    "<label for=\"pass\">Password:</label><br>"
    "<input type=\"password\" id=\"pass\" name=\"pass\"><br><br>"
    "<input type=\"submit\" value=\"Save and Reboot\">"
    "</form>"
    "</body>"
    "</html>";
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

char *url_decode_in_place(char *str) {
  // This is a simple in-place url-decode function.
  // A more robust version would handle more cases.
  char *a = str;
  char *b = str;
  while (*a) {
    if (*a == '%' && isxdigit((unsigned char)*(a + 1)) &&
        isxdigit((unsigned char)*(a + 2))) {
      char hex[3] = {*(a + 1), *(a + 2), 0};
      *b++ = strtol(hex, NULL, 16);
      a += 3;
    } else if (*a == '+') {
      *b++ = ' ';
      a++;
    } else {
      *b++ = *a++;
    }
  }
  *b = '\0';
  return str;
}

void handle_action(struct pbuf *p) {
  printf("%.*s\n", p->tot_len, p->payload);
  printf("testing [%.*s] against [%s]\n\n", p->payload, strlen(GEN_ACTION(up)),
         GEN_ACTION(up));
  // Checking if the request was a action
  if (strncmp(p->payload, GEN_ACTION(up), strlen(GEN_ACTION(up))) == 0) {
    // Move forward
    printf("[Action]Moving forward\n");
  } else if (strncmp(p->payload, GEN_ACTION(down), strlen(GEN_ACTION(down))) ==
             0) {
    // Move forward
    printf("[Action]Moving back\n");
  } else if (strncmp(p->payload, GEN_ACTION(left), strlen(GEN_ACTION(left))) ==
             0) {
    // Move forward
    printf("[Action]Moving left\n");
  } else if (strncmp(p->payload, GEN_ACTION(right),
                     strlen(GEN_ACTION(right))) == 0) {
    // Move forward
    printf("[Action]Moving right\n");
  }
}

char *http_response_content(char *html_body) {
  int body_len = strlen(html_body);

  (void)sprintf(http_headers,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %d\r\n" // Length of the body
                "Connection: close\r\n"  // Tell client to close connection
                "\r\n",                  // End of headers
                body_len);
  char *http_response =
      (char *)malloc(sizeof(char) * (strlen(http_headers) + body_len));
  printf("length of http response: %d\n", (int)strlen(http_headers) + body_len);
  strcpy(http_response, http_headers);
  strcpy(http_response + strlen(http_headers), html_body);
}

err_t tcp_server_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p,
                               err_t err) {
  if (p == NULL) {
    return tcp_close(tpcb);
  }
  tcp_recved(tpcb, p->tot_len);

  char *request = (char *)p->payload;

  if (!provision_mode) {
    if (err == ERR_OK) {
      tcp_recved(tpcb, p->tot_len);
      printf(" -- received %u bytes: \n", p->tot_len);

      // Responding
      // Dynamically adding the length of the body
      char *http_response = http_response_content(controller_html);
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
  // Handle GET / request - serve the configuration page
  else if (strncmp(request, "GET / ", strlen("GET / ")) == 0) {
    printf("Serving Wi-Fi configuration page.\n");
    // Construct and send the HTTP response with the form
    char http_header[256];
    sprintf(http_header,
            "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nConnection: "
            "close\r\nContent-Type: text/html\r\n\r\n",
            strlen(WIFI_PROVISIONING_HTML));
    tcp_write(tpcb, http_header, strlen(http_header), TCP_WRITE_FLAG_COPY);
    tcp_write(tpcb, WIFI_PROVISIONING_HTML, strlen(WIFI_PROVISIONING_HTML),
              TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
  }
  // Handle POST /save request - save credentials
  else if (strncmp(request, "POST /save ", strlen("POST /save ")) == 0) {
    printf("Received POST request to save credentials.\n");
    // Find the start of the body data (after the \r\n\r\n)
    char *body = strstr(request, "\r\n\r\n");
    if (body) {
      body += 4; // Move pointer past the \r\n\r\n

      // Simple parsing for ssid=...&pass=...
      char *ssid_ptr = strstr(body, "ssid=");
      char *pass_ptr = strstr(body, "pass=");

      if (ssid_ptr && pass_ptr) {
        ssid_ptr += 5; // Move past "ssid="
        pass_ptr += 5; // Move past "pass="

        // Terminate the SSID string at the '&'
        char *ampersand = strchr(ssid_ptr, '&');
        if (ampersand)
          *ampersand = '\0';

        // URL-decode the values in place
        url_decode_in_place(ssid_ptr);
        url_decode_in_place(pass_ptr);

        printf("Parsed SSID: '%s'\n", ssid_ptr);
        printf("Parsed Password: '%s'\n", pass_ptr);

        // Use your existing storage functions
        store_ssid(ssid_ptr);
        store_password(pass_ptr);

        // Send a success response and reboot
        const char *success_response =
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
            "<html><body><h1>Credentials Saved!</h1><p>The device will now "
            "reboot and connect to the new network.</p></body></html>";
        tcp_write(tpcb, success_response, strlen(success_response),
                  TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
        sleep_ms(100); // Give time for the response to be sent

        printf("Rebooting in 2 seconds...\n");
        sleep_ms(2000);
        watchdog_reboot(0, 0, 0);
      }
    }
  }

  pbuf_free(p);
  return tcp_close(tpcb);
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
