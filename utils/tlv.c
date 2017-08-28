#include "tlv.h"

tlv_t *new_tlv(tlv_type_t type, u32 len, void *value)
{
    tlv_t *tlv;
    
    if (!value || !len) {
        return NULL;
    }
    
    switch (type) {
    case TLV_INT:
        len = TLV_INT_LEN;
        break;
    case TLV_PTR:
        len = sizeof(value);
        break;
    case TLV_STR:
        len = strlen(value) + 1;
        break;
    case TLV_BIN:
        break;
    default:
        return 0;
    }

    tlv = (tlv_t *)malloc(sizeof *tlv + len);
    if (!tlv) {
        return NULL;
    }

    tlv->t = type;
    tlv->l = len;
    memcpy(tlv->v, value, len);

    return tlv;
}

int tlv_is_equal(tlv_t *a, tlv_t *b)
{
    if (!a || !b) {
        return a == b;
    }

    if (a->t != b->t || a->l != b->l) {
        return 0;
    }
    
    switch (a->t) {
    case TLV_INT:
        return (TLVINT(a)) == (TLVINT(b));
    case TLV_PTR:
        return (TLVPTR(a)) == (TLVPTR(b));
    case TLV_STR:
        return !strcmp(a->v, b->v);
    case TLV_BIN:
        return !memcmp(a->v, b->v, a->l);
    default:
        return 0;
    }
}

u32 tlv_hash(tlv_t *key)
{
    switch (key->t) {
    case TLV_INT:
    case TLV_PTR:
        return *(u32 *)(key->v);
    case TLV_STR:
        return bkdr_hash(key->v);
    case TLV_BIN:
        return bkdr_hash_bin(key->v, key->l);
    default:
        return 0;
    }
}

char *tlv_type2str(tlv_type_t t)
{
    switch (t) {
    case TLV_INT:
        return "<tlv.int>";
    case TLV_PTR:
        return "<tlv.pointer>";
    case TLV_STR:
        return "<tlv.string>";
    case TLV_BIN:
        return "<tlv.binary>";
    default:
        return "<tlv.unknown>";
    }
}

char *tlv2str(tlv_t *tlv)
{
    static char buf[1024];
    const u32 size = sizeof buf;

    if (!tlv) {
        return "<nil>";
    }
    
    switch (tlv->t) {
    case TLV_INT:
        snprintf(buf, size, "%ld", TLVINT(tlv));
        break;
    case TLV_PTR:
        snprintf(buf, size, "%p", TLVPTR(tlv));
        break;
    case TLV_STR:
        snprintf(buf, size, "%s", TLVSTR(tlv));
        break;
    case TLV_BIN:
        snprintf(buf, size, "<%p:%d>", tlv->v, tlv->l);
        break;
    default:
        snprintf(buf, size, "<%p:%d:%d>", tlv->v, tlv->l, tlv->t);
        break;
    }
    
    return buf;
}

