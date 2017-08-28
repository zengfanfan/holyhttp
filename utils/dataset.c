#include "dataset.h"

static data_subset_t *new_subset(void)
{
    data_subset_t *set = (data_subset_t *)malloc(sizeof *set);
    if (!set) {
        return NULL;
    }
    memset(set, 0, sizeof *set);
    dict_init(&set->children);
    set->value = NULL;
    return set;
}

static data_subset_t *get_sub(dataset_t *self, char *name)
{
    char *name_cp = name ? strdup(name) : NULL;
    char *item;
    data_subset_t *sub;
    void *pos;
    
    if (!self || !name) {
        FREE_IF_NOT_NULL(name_cp);
        return NULL;
    }

    sub = &self->sub;
    foreach_item_in_str(item, name_cp, ".") {
        pos = sub->children.get_sp(&sub->children, item);
        if (!pos) {
            free(name_cp);
            return NULL;
        }
        sub = pos;
    }

    free(name_cp);
    return sub;
}

static char *get_value(dataset_t *self, char *name)
{
    data_subset_t *sub = get_sub(self, name);
    if (!sub) {
        return NULL;
    }
    return sub->value ? sub->value : name;
}

static int set_value(dataset_t *self, char *name, char *value)
{
    char *name_cp = name ? strdup(name) : NULL;
    char *item;
    data_subset_t *sub, *new;
    void *pos;
    
    if (!self || !name || !value) {
        FREE_IF_NOT_NULL(name_cp);
        return 0;
    }

    sub = &self->sub;
    foreach_item_in_str(item, name_cp, ".") {
        pos = sub->children.get_sp(&sub->children, item);
        if (pos) {
            sub = pos;
            continue;
        }
        // new one
        new = new_subset();
        if (!new) {
            free(name_cp);
            return 0;
        }
        sub->children.set_sp(&sub->children, item, new);
        sub = new;
    }

    free(name_cp);
    FREE_IF_NOT_NULL(sub->value);
    sub->value = strdup(value);
    return !!sub->value;
}

static void clear_sub_values(data_subset_t *sub);
static void _free_each_child(tlv_t *key, tlv_t *value, void *args)
{
    data_subset_t *sub = (data_subset_t *)TLVPTR(value);
    clear_sub_values(sub);
    free(sub);
}

static void clear_sub_values(data_subset_t *sub)
{
    FREE_IF_NOT_NULL(sub->value);
    sub->children.foreach(&sub->children, _free_each_child, NULL);
}

static void clear_data_set(dataset_t *self)
{
    if (self) {
        clear_sub_values(&self->sub);
    }
}

static void show_sub_values(data_subset_t *sub, int indent);
static void _show_each_child(tlv_t *key, tlv_t *value, void *args)
{
    int indent = *(int *)args;
    if (indent) {
        PRINT_BLUE("%*s", indent+1, ".");
    }
    PRINT_BLUE("%s", TLVSTR(key));
    show_sub_values((data_subset_t *)TLVPTR(value), indent+1);
}

static void show_sub_values(data_subset_t *sub, int indent)
{
    if (sub->value) {
        PRINT_BLUE(" = %s", sub->value);
    }
    PRINT_BLUE("\n");
    sub->children.foreach(&sub->children, _show_each_child, &indent);
}

static void show_values(dataset_t *self)
{
    if (self) {
        show_sub_values(&self->sub, 0);
    }
}

static void _handle_each_value(tlv_t *key, tlv_t *value, void *args)
{
    void **params = (void **)args;
    ds_foreach_handler_t handler = (ds_foreach_handler_t)(params[0]);
    void *arg = params[1];
    data_subset_t *sub = TLVPTR(value);
    
    if (sub->value) {
        handler(sub->value, arg);
        return;
    }
}

static void foreach_by_name(dataset_t *self, char *name,
                ds_foreach_handler_t handler, void *args)
{
    data_subset_t *sub = get_sub(self, name);
    void *params[] = {handler, args};
    
    if (!sub || !handler) {
        return;
    }

    sub->children.foreach(&sub->children, _handle_each_value, params);
}

int dataset_init(dataset_t *self)
{
    if (!self) {
        return 0;
    }

    if (self->inited) {
        return 1;
    }

    memset(self, 0, sizeof *self);
    dict_init(&self->sub.children);

    self->get = get_value;
    self->set = set_value;
    self->show = show_values;
    self->foreach_by = foreach_by_name;
    self->clear = clear_data_set;

    self->inited = 1;
    return 1;
}

