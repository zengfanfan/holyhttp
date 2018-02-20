#ifndef HOLYHTTP_DICT_H
#define HOLYHTTP_DICT_H

#include <holyhttp.h>
#include "list.h"
#include "tlv.h"

#define DICT_HASH_SIZE  (1U << HOLY_HASH_BITS)

typedef void (*dict_foreach_handler_t)(tlv_t *key, tlv_t *value, void *args);
typedef struct dict {
    unsigned count;
    list_head_t kvs[DICT_HASH_SIZE];
    int inited;

    int (*set_ss)(struct dict *self, char *key, char *value);
    str_t (*get_ss)(struct dict *self, char *key);
    int (*set_sb)(struct dict *self, char *key, void *value, u32 vlen);
    int (*get_sb)(struct dict *self, char *key, void **value, u32 *vlen);
    int (*set_ii)(struct dict *self, long key, long value);
    long (*get_ii)(struct dict *self, long key);
    int (*set_ib)(struct dict *self, long key, void *value, u32 vlen);
    int (*get_ib)(struct dict *self, long key, void **value, u32 *vlen);
    int (*set_sp)(struct dict *self, char *key, void *value);
    void_ptr_t (*get_sp)(struct dict *self, char *key);
    int (*set_ip)(struct dict *self, long key, void *value);
    void_ptr_t (*get_ip)(struct dict *self, long key);
    void (*del_s)(struct dict *self, char *key);
    void (*del_i)(struct dict *self, long key);
    void (*clear)(struct dict *self);
    void (*foreach)(struct dict *self, dict_foreach_handler_t handler, void *args);
    void (*show)(struct dict *self);
} dict_t;

dict_t *holy_new_dict(void);
void holy_free_dict(dict_t *self);
int holy_dict_init(dict_t *self);

#endif // HOLYHTTP_DICT_H
