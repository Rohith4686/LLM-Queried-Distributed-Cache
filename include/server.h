#ifndef SERVER_H
#define SERVER_H

#include "cache.h"

#define DEFAULT_PORT      7370
#define DEFAULT_BACKLOG   128
#define MAX_PEERS         8
#define RECV_BUF_SIZE     (8192)

typedef struct {
    int         node_id;
    int         port;
    char        peers[MAX_PEERS][256];
    int         peer_count;
    cache_t    *cache;
} server_config_t;

int  server_init(server_config_t *cfg);
void server_run(server_config_t *cfg);
void server_shutdown(void);

#endif /* SERVER_H */
