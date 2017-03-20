#include "net/tcp.h"

#include "errno.h"
#include "console.h"
#include "memory.h"
#include "fs/vfs.h"

static void http_read(stream * stream, const uint8_t* request, uint32_t size) {
    // console_print_string((const char*)request);

    const char* h1 =  "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: ";
    static const char* seperator =  "\r\n\r\n";
    static uint32_t sep_size = 4;

    char body[256];
    int count = read("HELLO~1.HTM", body, sizeof(body));
    if (count < 0) {
        warn("Cannot load HELLO~1.HTM");
        return;
    }

    uint32_t total = count + strlen(h1) + sep_size * 2;
    char * response = (char*) kmem_alloc(total);
    strncpy(response, h1, strlen(h1));
    char * next = to_str(count, response + strlen(h1));
    strncpy(next, seperator, sep_size);
    next += strlen(seperator);
    strncpy(next, body, count);
    strncpy(next + count, seperator, sep_size);

    tcp_send(stream, response, total);
    tcp_close(stream);

    kmem_free(response);
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
