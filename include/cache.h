#ifndef CACHE_H
#define CACHE_H

#include <stddef.h>
#include <pthread.h>
#include <time.h>

#define CACHE_TABLE_SIZE  1024
#define CACHE_MAX_KEY_LEN  256
#define CACHE_MAX_VAL_LEN 4096

/* A single key-value entry in the cache */
typedef struct cache_entry {
    char             key[CACHE_MAX_KEY_LEN];
    char            *value;
    size_t           value_len;
    time_t           expires_at;   /* 0 = no expiry */
    struct cache_entry *next;      /* chained for collision resolution */
} cache_entry_t;

/* One bucket in the hash table — has its own lock for fine-grained concurrency */
typedef struct {
    cache_entry_t  *head;
    pthread_mutex_t lock;
} cache_bucket_t;

/* The cache itself */
typedef struct {
    cache_bucket_t  buckets[CACHE_TABLE_SIZE];
    unsigned long   hits;
    unsigned long   misses;
    unsigned long   total_keys;
} cache_t;

/* Lifecycle */
cache_t *cache_create(void);
void     cache_destroy(cache_t *c);

/* Operations */
int   cache_set(cache_t *c, const char *key, const char *value, size_t value_len, int ttl_secs);
char *cache_get(cache_t *c, const char *key, size_t *out_len);
int   cache_delete(cache_t *c, const char *key);
void  cache_flush(cache_t *c);

/* Stats */
void cache_stats(const cache_t *c, unsigned long *hits, unsigned long *misses, unsigned long *keys);

#endif /* CACHE_H */
