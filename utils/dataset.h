#ifndef HOLYHTTP_DATASET_H
#define HOLYHTTP_DATASET_H

/*
a {
    b {
        name: zff
        age: 28
    }
    c {
        0 {
            key: holy
            value: ship
        }
        1 {
            key: go
            value: die
        }
    }
}

a.b.name = zff
a.b.age = 28
a.c.0.key = holy
a.c.1.value = die
*/

#include "dict.h"

typedef void (*ds_foreach_handler_t)(char *value, void *args);

typedef struct {
    dict_t children;
    char *value;
} data_subset_t;

typedef struct dataset {
    int inited;
    data_subset_t sub;

    str_t (*get)(struct dataset *self, char *name);
    int (*set)(struct dataset *self, char *name, char *value);
    void (*show)(struct dataset *self);
    void (*foreach_by)(struct dataset *self, char *name, ds_foreach_handler_t handler, void *args);
    void (*clear)(struct dataset * sub);
} dataset_t;

int dataset_init(dataset_t *self);

#endif // HOLYHTTP_DATASET_H
