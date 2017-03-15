#include "net/tcp.h"

#include "errno.h"
#include "console.h"
#include "fs/vfs.h"

static void http_read(stream * stream, const uint8_t* request, uint32_t size) {
    // console_print_string((const char*)request);

    const char* h1 =  "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: ";
    const char* h2 =  "\r\n\r\n";

    char body[256];
    int count = read("HELLO~1.HTM", body, sizeof(body));
    if (count < 0) {
        warn("Cannot load HELLO~1.HTM");
        return;
    }

    char len[32];
    to_str(count, len);

    tcp_send(stream, h1, strlen(h1));
    tcp_send(stream, len, strlen(len));
    tcp_send(stream, h2, strlen(h2));
    tcp_send(stream, body, count);
    tcp_send(stream, h2, strlen(h2));
    tcp_close(stream);
}

static tcp_read_fn http_accept(stream * stream) {
    return http_read;
}

static void http_run() {
    if (EOK != listen(80, http_accept)) {
        console_print_string("Failed to listen on port 80\n");
    }
}

void init_http() {
    http_run(); // TODO user space
}
