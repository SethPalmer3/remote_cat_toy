#include "callbacks.h"
#include "hardware/sync.h"
#include "hardware/timer.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#include "storage.h"
#include "wheel_controller.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Volatile Globals

static volatile bool motor_action_in_progress = false;

// --- Constants ---
#define ACTION_PREFIX "GET /button?action="
#define ACTIVATE_TIME 200
#define LEFT_FORWARD 2
#define LEFT_BACKWARD 3
#define RIGHT_FORWARD 12
#define RIGHT_BACKWARD 13
#define DEBUG 0

// --- HTTP Content ---

// Use 'static const' for read-only string data.
static const char *HTTP_HEADER_200_OK = "HTTP/1.1 200 OK\r\n";
static const char *HTTP_HEADER_CONTENT_HTML = "Content-Type: text/html\r\n";
static const char *HTTP_HEADER_CONNECTION_CLOSE = "Connection: close\r\n";

static const char *WIFI_PROVISIONING_HTML =
    "<!DOCTYPE html>"
    "<html>"
    "<head><title>Pico W Wi-Fi Setup</title></head>"
    "<body>"
    "<h1>Wi-Fi Setup</h1>"
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

static const char *CONTROLLER_HTML =
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<title>Pico W Controller</title>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<style>"
    "body { font-family: Arial, sans-serif; text-align: center; margin-top: "
    "50px; user-select: none; }"
    "button { width: 100px; height: 100px; margin: 10px; font-size: 20px; "
    "border-radius: 10px; }"
    "#middle-row { display: flex; justify-content: center; align-items: "
    "center; }"
    "#buttonLeft { margin-right: 80px; }"
    "</style>"
    "<script>"
    "let actionIntervalId = null;"
    "const ACTION_RATE_MS = 200;"
    ""
    "function sendAction(action) {"
    "const slider_value = document.getElementById('durationSlider').value;"
    "  fetch('/button?action=' + action + '&duration=' + slider_value).catch(e "
    "=> console.error('Request "
    "failed:', e));"
    "}"
    ""
    "function startAction(action) {"
    "  if (actionIntervalId !== null) return;"
    "  sendAction(action);"
    "  actionIntervalId = setInterval(() => sendAction(action), "
    "ACTION_RATE_MS);"
    "}"
    ""
    "function stopAction() {"
    "  if (actionIntervalId === null) return;"
    "  clearInterval(actionIntervalId);"
    "  actionIntervalId = null;"
    "}"
    // Add event listeners that work for both mouse and touch
    "document.addEventListener('DOMContentLoaded', () => {"
    "  const slider = document.getElementById('durationSlider');"
    "  const display = document.getElementById('durationValue');"
    "  slider.addEventListener('input', () => {"
    "    display.textContent = slider.value;"
    "  });"
    "  const buttons = {"
    "    'buttonUp': 'up',"
    "    'buttonDown': 'down',"
    "    'buttonLeft': 'left',"
    "    'buttonRight': 'right',"
    "  };"
    "  for (const [id, action] of Object.entries(buttons)) {"
    "    const button = document.getElementById(id);"
    "    button.addEventListener('mousedown', () => startAction(action));"
    "    button.addEventListener('touchstart', (e) => { e.preventDefault(); "
    "startAction(action); });"
    "    button.addEventListener('mouseup', stopAction);"
    "    button.addEventListener('mouseleave', stopAction);"
    "    button.addEventListener('touchend', stopAction);"
    "  }"
    "});"
    "</script>"
    "</head>"
    "<body>"
    "<h1>Controller</h1>"
    "<div>"
    "  <label for=\"durationSlider\">Motor On-Time (ms):</label>"
    "  <input type=\"range\" id=\"durationSlider\" min=\"50\" max=\"1000\" "
    "value=\"200\" step=\"10\">"
    "  <span id=\"durationValue\">200</span>"
    "</div>"
    "<div><button id=\"buttonUp\">Up</button></div>"
    "<div id=\"middle-row\">"
    "<button id=\"buttonLeft\">Left</button>"
    "<button id=\"buttonRight\">Right</button>"
    "</div>"
    "<div><button id=\"buttonDown\">Down</button></div>"
    "<p>%s</p>"
    "</body>"
    "</html>";

// --- Utility Functions ---
/**
 * @brief This function is called by the hardware timer alarm to turn the motors
 * off.
 * @param id The ID of the alarm that triggered.
 * @param user_data User data passed to the alarm (not used here).
 * @return int64_t 0 to prevent the timer from repeating.
 */
int64_t end_motor_action_callback(alarm_id_t id, void *user_data) {
  printf("[Timer] Turning motors off.\n");
  // Stop all motors
  move(0, LEFT_FORWARD, LEFT_BACKWARD);
  move(0, RIGHT_FORWARD, RIGHT_BACKWARD);
  // Reset the flag to allow new actions
  motor_action_in_progress = false;
  return 0; // Do not repeat the timer
}
/**
 * @brief In-place URL-decodes a string.
 * @param str The string to decode.
 */
static void url_decode_in_place(char *str) {
  char *write_ptr = str;
  while (*str) {
    if (*str == '%' && isxdigit((unsigned char)*(str + 1)) &&
        isxdigit((unsigned char)*(str + 2))) {
      char hex[3] = {*(str + 1), *(str + 2), '\0'};
      *write_ptr++ = strtol(hex, NULL, 16);
      str += 3;
    } else if (*str == '+') {
      *write_ptr++ = ' ';
      str++;
    } else {
      *write_ptr++ = *str++;
    }
  }
  *write_ptr = '\0';
}

// --- HTTP Response Functions ---

/**
 * @brief Sends a simple HTTP response with only headers (e.g., 200 OK).
 */
static err_t send_simple_response(struct tcp_pcb *tpcb, const char *status) {
  err_t err = tcp_write(tpcb, status, strlen(status), TCP_WRITE_FLAG_COPY);
  if (err != ERR_OK)
    return err;
  return tcp_write(tpcb, HTTP_HEADER_CONNECTION_CLOSE,
                   strlen(HTTP_HEADER_CONNECTION_CLOSE), TCP_WRITE_FLAG_COPY);
}

/**
 * @brief Constructs and sends a full HTTP response with a body.
 */
static err_t send_full_response(struct tcp_pcb *tpcb, const char *body) {
  char headers[128];
  snprintf(headers, sizeof(headers), "%s%sContent-Length: %d\r\n%s\r\n",
           HTTP_HEADER_200_OK, HTTP_HEADER_CONTENT_HTML, strlen(body),
           HTTP_HEADER_CONNECTION_CLOSE);

  err_t err = tcp_write(tpcb, headers, strlen(headers), TCP_WRITE_FLAG_COPY);
  if (err != ERR_OK) {
    printf("Failed to write HTTP headers. Error: %d\n", err);
    return err;
  }

  err = tcp_write(tpcb, body, strlen(body), TCP_WRITE_FLAG_COPY);
  if (err != ERR_OK) {
    printf("Failed to write HTTP body. Error: %d\n", err);
  }
  return err;
}

// --- Logic Handlers ---

/**
 * @brief Handles the directional commands from the controller page.
 */
static void handle_action(const char *payload, int duration_ms) {
  if (motor_action_in_progress) {
    printf("[Action] Ignoring action, already in progress\n");
    return;
  }
  if (duration_ms < 20) {
    duration_ms = 20;
  }
  if (duration_ms > 2000) {
    duration_ms = 2000;
  }
  motor_action_in_progress = true;
  if (strncmp(payload, "up", strlen("up")) == 0) {
    printf("[Action] Moving forward for %d ms\n", duration_ms);
    move(1, LEFT_FORWARD, LEFT_BACKWARD);
    move(1, RIGHT_FORWARD, RIGHT_BACKWARD);
  } else if (strncmp(payload, "down", strlen("down")) == 0) {
    printf("[Action] Moving backward for %d ms\n", duration_ms);
    move(-1, LEFT_FORWARD, LEFT_BACKWARD);
    move(-1, RIGHT_FORWARD, RIGHT_BACKWARD);
  } else if (strncmp(payload, "left", strlen("left")) == 0) {
    printf("[Action] Moving left for %d ms\n", duration_ms);
    move(-1, LEFT_FORWARD, LEFT_BACKWARD);
    move(1, RIGHT_FORWARD, RIGHT_BACKWARD);
  } else if (strncmp(payload, "right", strlen("right")) == 0) {
    printf("[Action] Moving right for %d ms\n", duration_ms);
    move(1, LEFT_FORWARD, LEFT_BACKWARD);
    move(-1, RIGHT_FORWARD, RIGHT_BACKWARD);
  } else {
    printf("[Action] Unknown action (%s)", payload);
    motor_action_in_progress = 0;
  }
  add_alarm_in_ms(duration_ms, end_motor_action_callback, NULL, false);
}

/**
 * @brief Handles incoming requests when the device is in provisioning mode.
 */
static err_t handle_provisioning_request(struct tcp_pcb *tpcb,
                                         const char *request, uint16_t len) {
  // Serve the Wi-Fi setup page on GET /
  if (strncmp(request, "GET / ", 6) == 0) {
    printf("Serving Wi-Fi configuration page.\n");
    return send_full_response(tpcb, WIFI_PROVISIONING_HTML);
  }
  // Handle saving credentials on POST /save
  else if (strncmp(request, "POST /save ", 11) == 0) {
    printf("Received POST to save credentials.\n");
    const char *body = strstr(request, "\r\n\r\n");
    if (body) {
      body += 4; // Skip headers

      char *ssid = strstr(body, "ssid=");
      char *pass = strstr(body, "pass=");

      if (ssid && pass) {
        ssid += 5; // Move past "ssid="
        pass += 5; // Move past "pass="

        char *ampersand = strchr(ssid, '&');
        if (ampersand)
          *ampersand = '\0';

        url_decode_in_place(ssid);
        url_decode_in_place(pass);

        printf("Parsed SSID: '%s'\n", ssid);
        printf("Parsed Password: '%s'\n", pass);

        store_ssid(ssid);
        store_password(pass);

        const char *success_msg =
            "<h1>Credentials Saved!</h1><p>Device will now reboot.</p>";
        send_full_response(tpcb, success_msg);
        tcp_output(tpcb);

        sleep_ms(2000);
        watchdog_reboot(0, 0, 0);
      }
    }
  }
  return ERR_OK;
}

/**
 * @brief Handles incoming requests when the device is in normal controller
 * mode.
 */
static err_t handle_controller_request(struct tcp_pcb *tpcb,
                                       const char *request, uint16_t len) {
  // Check if it's a button action

  if (DEBUG) {

    printf(" --- Request ---\n%s\n --- End Request ---\n", request);
  }
  if (strncmp(request, "GET /button?", 12) == 0) {

    // Find the action parameter
    const char *action_start = strstr(request, "action=");
    if (action_start) {
      action_start += 7; // Move pointer past "action="

      // Find the end of the action value (it will be followed by a space)
      const char *action_end = action_start;
      while (*action_end && *action_end != ' ' && *action_end != '&') {
        action_end++;
      }

      size_t action_len = action_end - action_start;
      char action_str[16];

      // Safely copy the action string
      if (action_len > 0 && action_len < sizeof(action_str)) {
        strncpy(action_str, action_start, action_len);
        action_str[action_len] = '\0'; // Ensure it's null-terminated

        // Find the duration parameter, default to 200ms
        int duration = 200;
        const char *duration_start = strstr(request, "duration=");
        // printf("found duration: %s\n", duration_start);
        if (duration_start) {
          duration_start += 9; // Move pointer past "duration="
          duration = atoi(duration_start);
        }

        // Call the handle_action function
        handle_action(action_str, duration);
      }
    }

    return send_simple_response(tpcb, HTTP_HEADER_200_OK);
  }

  // Otherwise, serve the main controller page
  else if (strncmp(request, "GET / ", 6) == 0) {
    struct netif *netif = netif_default;
    const ip4_addr_t *ip_addr = netif_ip4_addr(netif);
    char *modified_cntrl_html =
        malloc(strlen(CONTROLLER_HTML) + strlen(ip4addr_ntoa(ip_addr)));
    sprintf(modified_cntrl_html, CONTROLLER_HTML, ip4addr_ntoa(ip_addr));
    err_t e = send_full_response(tpcb, modified_cntrl_html);
    free(modified_cntrl_html);
    return e;
  }

  return ERR_OK;
}

// --- lwIP Callback Functions ---

err_t tcp_server_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p,
                               err_t err) {
  if (p == NULL) {
    printf("Connection closed by client.\n");
    return tcp_close(tpcb);
  }

  if (err != ERR_OK) {
    printf("Receive error: %d\n", err);
    pbuf_free(p);
    return err;
  }

  // Acknowledge receipt of the data
  tcp_recved(tpcb, p->tot_len);

  char *request = (char *)p->payload;

  // Route request based on the device mode
  if (provision_mode) {
    handle_provisioning_request(tpcb, request, p->tot_len);
  } else {
    handle_controller_request(tpcb, request, p->tot_len);
  }

  pbuf_free(p);
  return tcp_close(tpcb);
}

err_t tcp_server_accept_callback(void *arg, struct tcp_pcb *new_pcb,
                                 err_t err) {
  if (err != ERR_OK || new_pcb == NULL) {
    printf("Accept callback error: %d\n", err);
    return ERR_VAL;
  }

  printf("Client connected.\n");
  tcp_accepted(new_pcb);
  tcp_recv(new_pcb, tcp_server_recv_callback);

  return ERR_OK;
}
