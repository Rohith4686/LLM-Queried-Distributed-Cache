#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>

#include "server.h"
#include "cache.h"

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [options]\n"
        "\n"
        "Options:\n"
        "  --port    <port>       Listen port (default: %d)\n"
        "  --node-id <id>         Node ID (0, 1, 2, ...)\n"
        "  --peers   <host:port>  Comma-separated peer list\n"
        "  -h, --help             Show this help\n"
        "\n"
        "Example:\n"
        "  %s --port 7370 --node-id 0\n"
        "  %s --port 7371 --node-id 1 --peers localhost:7370\n",
        prog, DEFAULT_PORT, prog, prog);
    exit(1);
}

static void parse_peers(server_config_t *cfg, const char *peers_str) {
    char buf[2048];
    strncpy(buf, peers_str, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *token = strtok(buf, ",");
    while (token && cfg->peer_count < MAX_PEERS) {
        strncpy(cfg->peers[cfg->peer_count], token, 255);
        cfg->peers[cfg->peer_count][255] = '\0';
        cfg->peer_count++;
        token = strtok(NULL, ",");
    }
}

int main(int argc, char *argv[]) {
    server_config_t cfg = {
        .node_id    = 0,
        .port       = DEFAULT_PORT,
        .peer_count = 0,
    };

    static struct option long_opts[] = {
        { "port",    required_argument, 0, 'p' },
        { "node-id", required_argument, 0, 'n' },
        { "peers",   required_argument, 0, 'P' },
        { "help",    no_argument,       0, 'h' },
        { 0, 0, 0, 0 }
    };

    int opt, idx = 0;
    while ((opt = getopt_long(argc, argv, "p:n:P:h", long_opts, &idx)) != -1) {
        switch (opt) {
            case 'p': cfg.port    = atoi(optarg);       break;
            case 'n': cfg.node_id = atoi(optarg);       break;
            case 'P': parse_peers(&cfg, optarg);         break;
            case 'h':
            default:  usage(argv[0]);
        }
    }

    printf("[node %d] starting on port %d with %d peer(s)\n",
           cfg.node_id, cfg.port, cfg.peer_count);

    cfg.cache = cache_create();
    if (!cfg.cache) {
        fprintf(stderr, "fatal: failed to allocate cache\n");
        return 1;
    }

    if (server_init(&cfg) != 0) {
        fprintf(stderr, "fatal: server_init failed\n");
        cache_destroy(cfg.cache);
        return 1;
    }

    server_run(&cfg);

    cache_destroy(cfg.cache);
    return 0;
}
