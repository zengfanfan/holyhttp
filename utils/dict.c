#include "dict.h"

#define DICT_HASH_MASK  (DICT_HASH_SIZE - 1)
#define DICT_HASH(key)  (holy_tlv_hash(key) & DICT_HASH_MASK)

typedef struct {
    list_head_t link;
    tlv_t *key;
    tlv_t *value;
} dict_kv_t;

static dict_kv_t *dict_find(dict_t *self, tlv_t *key)
{
    dict_kv_t *pos;

    //DEBUG("key: %d %u", key->t, key->l);
    list_for_each_entry(pos, &self->kvs[DICT_HASH(key)], link) {
        if (holy_tlv_is_equal(pos->key, key)) {
            return pos;
        }
    }

    return NULL;
}

static void dict_rm_kv(dict_t *self, dict_kv_t *kv)
{
    list_del(&kv->link);
    FREE_IF_NOT_NULL(kv->key);
    FREE_IF_NOT_NULL(kv->value);
    FREE_IF_NOT_NULL(kv);
    --self->count;
}

static tlv_t *dict_get(dict_t *self, tlv_t *key)
{
    dict_kv_t *kv;
    
    // find
    kv = dict_find(self, key);
    if (!kv) {
        return NULL;
    }

    return kv->value;
}

static void dict_foreach(dict_t *self, dict_foreach_handler_t handler, void *args)
{
    dict_kv_t *pos, *n;
    int i;

    if (!self || !handler) {
        return;
    }
    
    for (i = 0; i < DICT_HASH_SIZE; ++i) {
        list_for_each_entry_safe(pos, n, &self->kvs[i], link) {
            handler(pos->key, pos->value, args);
        }
    }
}

static void dict_clear(dict_t *self)
{
    dict_kv_t *pos, *n;
    int i;
    
    for (i = 0; i < DICT_HASH_SIZE; ++i) {
        list_for_each_entry_safe(pos, n, &self->kvs[i], link) {
            dict_rm_kv(self, pos);
        }
    }
}

static void dict_del(dict_t *self, tlv_t *key)
{
    dict_kv_t *kv;
    
    // find
    kv = dict_find(self, key);
    if (!kv) {
        return;
    }

    dict_rm_kv(self, kv);
}

static int dict_set(dict_t *self, tlv_t *key, tlv_t *value)
{
    dict_kv_t *old, *new;
    
    // new
    new = (dict_kv_t *)malloc(sizeof *new);
    if (!new) {
        MEMFAIL();
        return 0;
    }

    // old
    old = dict_find(self, key);
    if (old) {
        dict_rm_kv(self, old);
    }
    
    // set
    new->key = key;
    new->value = value;
    list_add_tail(&new->link, &self->kvs[DICT_HASH(key)]);
    ++self->count;

    return 1;
}

static int set_type_type(dict_t *self,
    tlv_type_t kt, unsigned kl, void *kv,
    tlv_type_t vt, unsigned vl, void *vv
    )
{
    tlv_t *k, *v;
    
    if (!self) {
        return 0;
    }

    k = holy_new_tlv(kt, kl, kv);
    v = holy_new_tlv(vt, vl, vv);

    if (!k || !v) {
        FREE_IF_NOT_NULL(k);
        FREE_IF_NOT_NULL(v);
        return 0;
    }
    
    if (!dict_set(self, k, v)) {
        FREE_IF_NOT_NULL(k);
        FREE_IF_NOT_NULL(v);
        return 0;
    }

    return 1;
}

static tlv_t *get_type_type(dict_t *self, tlv_type_t kt, unsigned kl, void *kv)
{
    tlv_t *k, *v;
    
    if (!self) {
        return NULL;
    }

    k = holy_new_tlv(kt, kl, kv);
    if (!k) {
        return NULL;
    }

    v = dict_get(self, k);
    free(k);
    return v;
}

static void del_by_type(dict_t *self, tlv_type_t kt, unsigned kl, void *kv)
{
    tlv_t *k;
    
    if (!self) {
        return;
    }

    k = holy_new_tlv(kt, kl, kv);
    if (!k) {
        return;
    }

    dict_del(self, k);
    free(k);
}

static int set_str_str(dict_t *self, char *key, char *value)
{
    if (!key || !value) {
        return 0;
    }

    return set_type_type(self,
        TLV_STR, strlen(key) + 1, key,
        TLV_STR, strlen(value) + 1, value);
}

static char *get_str_str(dict_t *self, char *key)
{
    tlv_t *v;
    
    if (!key) {
        return NULL;
    }
    
    v = get_type_type(self, TLV_STR, strlen(key) + 1, key);
    return TLVSTR(v);
}

static int set_str_bin(dict_t *self, char *key, void *value, uint vlen)
{
    if (!key || !value) {
        return 0;
    }
    
    return set_type_type(self,
        TLV_STR, strlen(key) + 1, key,
        TLV_BIN, vlen, value);
}

static int get_str_bin(dict_t *self, char *key, void **value, u32 *vlen)
{
    tlv_t *v;
    
    if (!key || !value) {
        return 0;
    }
    
    v = get_type_type(self, TLV_STR, strlen(key) + 1, key);
    if (!v) {
        return 0;
    }

    *value = v->v;
    if (vlen) {
        *vlen = v->l;
    }
    return 1;
}

static int set_str_ptr(dict_t *self, char *key, void *value)
{
    if (!key) {
        return 0;
    }
    
    return set_type_type(self,
        TLV_STR, strlen(key) + 1, key,
        TLV_PTR, sizeof value, &value);
}

static void *get_str_ptr(dict_t *self, char *key)
{
    tlv_t *v;
    
    if (!key) {
        return 0;
    }
    
    v = get_type_type(self, TLV_STR, strlen(key) + 1, key);
    return TLVPTR(v);
}

static int set_int_int(dict_t *self, long key, long value)
{
    return set_type_type(self,
        TLV_INT, sizeof key, &key,
        TLV_INT, sizeof(value), &value);
}

static long get_int_int(dict_t *self, long key)
{
    tlv_t *v = get_type_type(self, TLV_INT, sizeof key, &key);
    return TLVINT(v);
}

static int set_int_bin(dict_t *self, long key, void *value, uint vlen)
{
    if (!value || !vlen) {
        return 0;
    }
    
    return set_type_type(self,
        TLV_INT, sizeof key, &key,
        TLV_BIN, vlen, value);
}

static int get_int_bin(dict_t *self, long key, void **value, u32 *vlen)
{
    tlv_t *v;
    
    if (!value) {
        return 0;
    }
    
    v = get_type_type(self, TLV_INT, sizeof key, &key);
    if (!v) {
        return 0;
    }
    
    *value = v->v;
    if (vlen) {
        *vlen = v->l;
    }
    return 1;
}

static int set_int_ptr(dict_t *self, long key, void *value)
{
    return set_type_type(self,
        TLV_INT, sizeof key, &key,
        TLV_PTR, sizeof value, &value);
}

static void *get_int_ptr(dict_t *self, long key)
{
    tlv_t *v = get_type_type(self, TLV_INT, sizeof key, &key);
    return TLVPTR(v);
}

static void del_by_str(dict_t *self, char *key)
{
    if (key) {
        del_by_type(self, TLV_STR, strlen(key) + 1, key);
    }
}

static void del_by_int(dict_t *self, long key)
{
    if (key) {
        del_by_type(self, TLV_INT, sizeof key, &key);
    }
}

static void dict_show(dict_t *self)
{
    int i, j;
    dict_kv_t *pos, *n;

    if (!self) {
        return;
    }
    
    for (i = j = 0; i < DICT_HASH_SIZE; ++i) {
        list_for_each_entry_safe(pos, n, &self->kvs[i], link) {
            PRINT_CYAN("[%d.%d]: %s", i, j++, holy_tlv2str(pos->key));
            PRINT_CYAN(" = %s\n", holy_tlv2str(pos->value));
        }
    }
}

int holy_dict_init(dict_t *self)
{
    int i;

    if (!self) {
        return 0;
    }
    
    if (self->inited) {
        return 1;
    }

    memset(self, 0, sizeof *self);
    for (i = 0; i < DICT_HASH_SIZE; ++i) {
        INIT_LIST_HEAD(&self->kvs[i]);
    }

    self->get_ss = get_str_str;
    self->set_ss = set_str_str;
    self->get_sb = get_str_bin;
    self->set_sb = set_str_bin;
    self->get_sp = get_str_ptr;
    self->set_sp = set_str_ptr;
    self->get_ii = get_int_int;
    self->set_ii = set_int_int;
    self->get_ib = get_int_bin;
    self->set_ib = set_int_bin;
    self->get_ip = get_int_ptr;
    self->set_ip = set_int_ptr;
    self->del_s = del_by_str;
    self->del_i = del_by_int;
    self->clear = dict_clear;
    self->foreach = dict_foreach;
    self->show = dict_show;

    self->inited = 1;

    return 1;
}

void holy_free_dict(dict_t *self)
{
    if (self) {
        self->clear(self);
        free(self);
    }
}

dict_t *holy_new_dict(void)
{
    dict_t *self = (dict_t *)malloc(sizeof *self);
    if (!self) {
        MEMFAIL();
        return NULL;
    }
    memset(self, 0, sizeof *self);
    holy_dict_init(self);
    return self;
}

