#ifndef HOLYHTTP_TLV_H
#define HOLYHTTP_TLV_H

#include <stdlib.h>
#include "print.h"
#include "string.h"

typedef enum {
    TLV_INT,
    TLV_STR,
    TLV_PTR,
    TLV_BIN,
} tlv_type_t;

typedef struct {
    tlv_type_t t;
    u32 l;
    char v[0];
} tlv_t;

#define TLV_INT_TYPE long
#define TLV_INT_LEN sizeof(TLV_INT_TYPE)

#define TLVSTR(tlv) (char *)((tlv) ? (char *)((tlv)->v) : "")
#define TLVINT(tlv) (long)((tlv) ? *(TLV_INT_TYPE *)((tlv)->v) : 0L)
#define TLVPTR(tlv) (void *)TLVINT(tlv)

tlv_t *new_tlv(tlv_type_t type, u32 len, void *value);
int tlv_is_equal(tlv_t *a, tlv_t *b);
u32 tlv_hash(tlv_t *key);
char *tlv_type2str(tlv_type_t t);
char *tlv2str(tlv_t *tlv);

#endif // HOLYHTTP_TLV_H
