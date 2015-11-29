#include "net/tcp.h"

#include "errno.h"
#include "console.h"

static void http_read(stream * stream, const uint8_t* body, uint32_t size) {
    //console_print_string((const char*)body);

    const char* response =  "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 26\r\n\r\n<html>Bad-OS 64</html>\r\n\r\n";
    tcp_send(stream, response, strlen(response));
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
    http_run();
}
