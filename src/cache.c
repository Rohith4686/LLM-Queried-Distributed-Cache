#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cache.h"

/* djb2 hash */
static unsigned long hash_key(const char *key) {
    unsigned long h = 5381;
    int c;
    while ((c = (unsigned char)*key++))
        h = ((h << 5) + h) + c;
    return h;
}

static size_t bucket_index(const char *key) {
    return hash_key(key) % CACHE_TABLE_SIZE;
}

cache_t *cache_create(void) {
    cache_t *c = calloc(1, sizeof(cache_t));
    if (!c) return NULL;

    for (int i = 0; i < CACHE_TABLE_SIZE; i++) {
        c->buckets[i].head = NULL;
        pthread_mutex_init(&c->buckets[i].lock, NULL);
    }
    return c;
}

void cache_destroy(cache_t *c) {
    if (!c) return;
    cache_flush(c);
    for (int i = 0; i < CACHE_TABLE_SIZE; i++)
        pthread_mutex_destroy(&c->buckets[i].lock);
    free(c);
}

int cache_set(cache_t *c, const char *key, const char *value, size_t value_len, int ttl_secs) {
    if (!c || !key || !value) return -1;
    if (strlen(key) >= CACHE_MAX_KEY_LEN) return -1;

    size_t idx = bucket_index(key);
    cache_bucket_t *bucket = &c->buckets[idx];

    pthread_mutex_lock(&bucket->lock);

    /* Check if key already exists — update in place */
    for (cache_entry_t *e = bucket->head; e; e = e->next) {
        if (strcmp(e->key, key) == 0) {
            char *new_val = malloc(value_len + 1);
            if (!new_val) { pthread_mutex_unlock(&bucket->lock); return -1; }
            memcpy(new_val, value, value_len);
            new_val[value_len] = '\0';
            free(e->value);
            e->value     = new_val;
            e->value_len = value_len;
            e->expires_at = ttl_secs > 0 ? time(NULL) + ttl_secs : 0;
            pthread_mutex_unlock(&bucket->lock);
            return 0;
        }
    }

    /* New entry */
    cache_entry_t *entry = calloc(1, sizeof(cache_entry_t));
    if (!entry) { pthread_mutex_unlock(&bucket->lock); return -1; }

    strncpy(entry->key, key, CACHE_MAX_KEY_LEN - 1);
    entry->value = malloc(value_len + 1);
    if (!entry->value) { free(entry); pthread_mutex_unlock(&bucket->lock); return -1; }
    memcpy(entry->value, value, value_len);
    entry->value[value_len] = '\0';
    entry->value_len  = value_len;
    entry->expires_at = ttl_secs > 0 ? time(NULL) + ttl_secs : 0;

    /* Prepend to chain */
    entry->next   = bucket->head;
    bucket->head  = entry;

    c->total_keys++;
    pthread_mutex_unlock(&bucket->lock);
    return 0;
}

char *cache_get(cache_t *c, const char *key, size_t *out_len) {
    if (!c || !key) return NULL;

    size_t idx = bucket_index(key);
    cache_bucket_t *bucket = &c->buckets[idx];

    pthread_mutex_lock(&bucket->lock);

    for (cache_entry_t *e = bucket->head; e; e = e->next) {
        if (strcmp(e->key, key) != 0) continue;

        /* Check TTL */
        if (e->expires_at > 0 && time(NULL) > e->expires_at) {
            /* Expired — treat as miss, reaper will clean it up */
            c->misses++;
            pthread_mutex_unlock(&bucket->lock);
            return NULL;
        }

        /* Return a caller-owned copy */
        char *copy = malloc(e->value_len + 1);
        if (!copy) { pthread_mutex_unlock(&bucket->lock); return NULL; }
        memcpy(copy, e->value, e->value_len + 1);
        if (out_len) *out_len = e->value_len;
        c->hits++;
        pthread_mutex_unlock(&bucket->lock);
        return copy;
    }

    c->misses++;
    pthread_mutex_unlock(&bucket->lock);
    return NULL;
}

int cache_delete(cache_t *c, const char *key) {
    if (!c || !key) return -1;

    size_t idx = bucket_index(key);
    cache_bucket_t *bucket = &c->buckets[idx];

    pthread_mutex_lock(&bucket->lock);

    cache_entry_t *prev = NULL;
    for (cache_entry_t *e = bucket->head; e; prev = e, e = e->next) {
        if (strcmp(e->key, key) != 0) continue;

        if (prev) prev->next  = e->next;
        else      bucket->head = e->next;

        free(e->value);
        free(e);
        c->total_keys--;
        pthread_mutex_unlock(&bucket->lock);
        return 0;
    }

    pthread_mutex_unlock(&bucket->lock);
    return -1; /* not found */
}

void cache_flush(cache_t *c) {
    if (!c) return;
    for (int i = 0; i < CACHE_TABLE_SIZE; i++) {
        pthread_mutex_lock(&c->buckets[i].lock);
        cache_entry_t *e = c->buckets[i].head;
        while (e) {
            cache_entry_t *next = e->next;
            free(e->value);
            free(e);
            e = next;
        }
        c->buckets[i].head = NULL;
        pthread_mutex_unlock(&c->buckets[i].lock);
    }
    c->total_keys = 0;
}

void cache_stats(const cache_t *c, unsigned long *hits, unsigned long *misses, unsigned long *keys) {
    if (!c) return;
    if (hits)   *hits   = c->hits;
    if (misses) *misses = c->misses;
    if (keys)   *keys   = c->total_keys;
}
