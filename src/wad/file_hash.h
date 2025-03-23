#ifndef FILE_HASH_H_INCLUDED
#define FILE_HASH_H_INCLUDED

#include "typedefs.h"

#define KEY_MAX_SIZE 8

typedef struct _bucket
{
    uint32_t value;
    char key[KEY_MAX_SIZE];
    bool has_value;
} bucket_t;

typedef struct _file_hash
{
    bucket_t *buckets;
    uint32_t capacity, size;
} file_hash_t;

file_hash_t fh_create_hash(uint32_t capacity);
void fh_insert_hash(file_hash_t *file_hash, const char *key, uint32_t value);
bool fh_get_value(file_hash_t *file_hash, const char *key, uint32_t *value);
void fh_delete_hash(file_hash_t *file_hash);

#endif