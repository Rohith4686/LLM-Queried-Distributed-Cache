#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>

#include "server.h"
#include "protocol.h"
#include "cache.h"

static volatile int g_running = 1;
static int          g_server_fd = -1;

static void handle_sigint(int sig) {
    (void)sig;
    g_running = 0;
    if (g_server_fd >= 0) close(g_server_fd);
}

int server_init(server_config_t *cfg) {
    struct sockaddr_in addr = {0};
    int fd, opt = 1;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return -1; }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((uint16_t)cfg->port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(fd); return -1;
    }
    if (listen(fd, DEFAULT_BACKLOG) < 0) {
        perror("listen"); close(fd); return -1;
    }

    g_server_fd = fd;
    signal(SIGINT,  handle_sigint);
    signal(SIGTERM, handle_sigint);

    printf("[node %d] listening on 0.0.0.0:%d\n", cfg->node_id, cfg->port);
    return 0;
}

/* Placeholder client handler — will be replaced with threaded version in commit 5 */
static void handle_client(int client_fd, server_config_t *cfg) {
    char buf[RECV_BUF_SIZE];
    proto_frame_t frame;
    size_t out_len;
    char *val;

    while (1) {
        int rc = proto_read_frame(client_fd, &frame, buf, sizeof(buf));
        if (rc <= 0) break;   /* client disconnected or error */

        switch (frame.header.op_code) {
            case OP_SET:
                cache_set(cfg->cache, frame.key, frame.value,
                          frame.header.val_len, 0);
                proto_write_frame(client_fd, OP_ACK, STATUS_OK,
                                  frame.header.request_id,
                                  NULL, 0, "OK", 2);
                break;

            case OP_GET:
                val = cache_get(cfg->cache, frame.key, &out_len);
                if (val) {
                    proto_write_frame(client_fd, OP_ACK, STATUS_OK,
                                      frame.header.request_id,
                                      frame.key, frame.header.key_len,
                                      val, (uint32_t)out_len);
                    free(val);
                } else {
                    proto_write_frame(client_fd, OP_ACK, STATUS_NOT_FOUND,
                                      frame.header.request_id,
                                      frame.key, frame.header.key_len,
                                      NULL, 0);
                }
                break;

            case OP_DELETE:
                cache_delete(cfg->cache, frame.key);
                proto_write_frame(client_fd, OP_ACK, STATUS_OK,
                                  frame.header.request_id,
                                  NULL, 0, "OK", 2);
                break;

            case OP_STATS: {
                unsigned long hits, misses, keys;
                cache_stats(cfg->cache, &hits, &misses, &keys);
                char stats_buf[256];
                int n = snprintf(stats_buf, sizeof(stats_buf),
                                 "hits=%lu misses=%lu keys=%lu",
                                 hits, misses, keys);
                proto_write_frame(client_fd, OP_ACK, STATUS_OK,
                                  frame.header.request_id,
                                  NULL, 0, stats_buf, (uint32_t)n);
                break;
            }

            default:
                proto_write_frame(client_fd, OP_ACK, STATUS_ERROR,
                                  frame.header.request_id,
                                  NULL, 0, "unknown op", 10);
                break;
        }
    }
    close(client_fd);
}

void server_run(server_config_t *cfg) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (g_running) {
        int client_fd = accept(g_server_fd,
                               (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (g_running) perror("accept");
            break;
        }
        printf("[node %d] connection from %s\n",
               cfg->node_id, inet_ntoa(client_addr.sin_addr));
        handle_client(client_fd, cfg);
    }

    printf("[node %d] shutting down\n", cfg->node_id);
}

void server_shutdown(void) {
    g_running = 0;
    if (g_server_fd >= 0) { close(g_server_fd); g_server_fd = -1; }
}
