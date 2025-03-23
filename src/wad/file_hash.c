#include "file_hash.h"
#include <string.h>

static uint32_t fh_hash_function(const char *key, uint32_t capacity)
{
    unsigned int hash = 5381;
    for (int i = 0; i < KEY_MAX_SIZE && key[i] != '\0'; i++)
        hash = ((hash << 5) + hash) + key[i];  // hash * 33 + key[i]

    return hash % capacity;
}

file_hash_t fh_create_hash(uint32_t capacity)
{
    file_hash_t file_hash = { .buckets = NULL, .capacity = capacity, .size = 0 };

    file_hash.buckets = (bucket_t*)malloc(capacity * sizeof(bucket_t));

    if (file_hash.buckets != NULL)
        memset(file_hash.buckets, 0, capacity * sizeof(bucket_t));

    return file_hash;
}

void fh_insert_hash(file_hash_t *file_hash, const char *key, uint32_t value)
{
    if (file_hash->size >= file_hash->capacity) return;

    uint32_t index = fh_hash_function(key, file_hash->capacity);
    
    for (uint32_t i = 0; i < file_hash->capacity; i++)
    {
        uint32_t id = (index + i) % file_hash->capacity;
        if (!file_hash->buckets[id].has_value)
        {
            file_hash->buckets[id].has_value = true;
            file_hash->buckets[id].value = value;
            memcpy(file_hash->buckets[id].key, key, KEY_MAX_SIZE);
            file_hash->size++;
            return;
        }
    }
}

bool fh_get_value(file_hash_t *file_hash, const char *key, uint32_t *value)
{
    uint32_t index = fh_hash_function(key, file_hash->capacity);
    
    for (uint32_t i = 0; i < file_hash->capacity; i++)
    {
        uint32_t id = (index + i) % file_hash->capacity;
        if (!file_hash->buckets[id].has_value) break;
        else if (strcmp(key, file_hash->buckets[id].key) == 0)
        {
            if (value != NULL)
                *value = file_hash->buckets[id].value;
            return true;
        }
    }

    return false;
}

void fh_delete_hash(file_hash_t *file_hash)
{
    if (file_hash->buckets != NULL)
        free(file_hash->buckets);
}
