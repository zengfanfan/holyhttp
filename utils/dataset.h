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

typedef void (*ds_foreach_handler_t)(int leaf, void *value, void *args);

typedef struct data_subset {
    dict_t children;
    char *value;
} data_subset_t;
typedef data_subset_t *data_subset_ptr_t;

typedef struct dataset {
    data_subset_t sub;
    int inited;

    str_t (*get)(struct dataset *self, char *name);
    int (*set)(struct dataset *self, char *name, char *value);
    int (*set_batch)(struct dataset *self, char *nvs, char *separator);
    void (*show)(struct dataset *self);
    void (*foreach_by)(struct dataset *self, char *name, ds_foreach_handler_t handler, void *args);
    void (*clear)(struct dataset * sub);
} dataset_t;

int dataset_init(dataset_t *self);

#endif // HOLYHTTP_DATASET_H
