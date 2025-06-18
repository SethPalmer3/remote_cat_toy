#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "lwip/tcp.h"

extern int provision_mode;

err_t tcp_server_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p,
                               err_t err);
err_t tcp_server_accept_callback(void *arg, struct tcp_pcb *new_pcb, err_t err);

#endif // !CALLBACKS_H
