#include "service/echo.h"

#include "net/udp.h"
#include "net/tcp.h"
#include "errno.h"

#include "console.h"

static void tcp_echo_read(stream* stream, const uint8_t* data, uint32_t sz) {
    tcp_send(stream, data, sz);
}

static tcp_read_fn tcp_echo_accept(stream * s) {
    return tcp_echo_read;
}

static void udp_echo_notify(const udp_quad * quad, const uint8_t* data, uint32_t sz) {
    udp_quad reverse = {
        .src_port = quad->dst_port, .src_addr = quad->dst_addr,
        .dst_port = quad->src_port, .dst_addr = quad->src_addr };
    int rc;
    if (EOK != (rc = udp_send(&reverse, data, sz))) {
        warn("Could not send UDP.");
    }
}

void init_echo() {
    if (EOK != tcp_listen(7, tcp_echo_accept)) {
        warn("Cannot listen on port 7 for tcp echo");
    }

    if (EOK != udp_listen(7, udp_echo_notify)) {
        warn("Cannot listen on port 7 for udp echo");
    }
}
