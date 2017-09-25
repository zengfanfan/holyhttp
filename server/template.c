#include <utils/print.h>
#include <utils/file.h>
#include "timer.h"
#include "template.h"

typedef enum {
    IF_OP_INVALID,
    IF_OP_EQ,//= equal
    IF_OP_NE,//!= not equal
    IF_OP_GT,//> greater than
    IF_OP_LT,//< less than
    IF_OP_GE,//>=
    IF_OP_LE,//<=
    IF_OP_NUM
} if_op_t;

typedef char *(*render_t)(void *, dataset_t *);

typedef struct {
    list_head_t link;
    render_t render;
} base_elem_t;

/*
# name1: @aa[.bb][@]
# name2: @{aa[.bb]}
# name3: @{aa bb + c.d ... (anything)}
# name4: @{{anything}}
# raw1: #raw{anything}
# raw2: #raw{{anything}}
# cmd1: #exec{anything}
# cmd2: #exec{{anything}}
*/
typedef struct {
    base_elem_t base;
    list_head_t children;
    char *text;// if there is no children
} normal_elem_t, template_t;

/*
<ul>
#if cond:
    yes
#else:
    no
#end
</ul>
*/
typedef struct {
    base_elem_t base;
    char *left;
    char *right;
    if_op_t op;
    normal_elem_t *yes, *no;
    char data[0];
} if_elem_t;

/*
<ul>
#for item in list:
    <li>@item</li>
#end
</ul>
*/
typedef struct {
    base_elem_t base;
    char *item, *list;
    normal_elem_t *body;
    char data[0];
} for_elem_t;

#define TMPL_NAME_LEN   100
#define TMPL_OP_LEN     3// > < >= <= == !=
#define TMPL_FMT_LEN    100
#define FOR_ITEM_NAME_PREFIX    ":for:"


//==========================================================
static dict_t cache = {.inited=0};

static int parse_normal_elem(char *text, u32 len, char **pnext, normal_elem_t **pelem);
static char *render_normal_elem(void *elem, dataset_t *ds);

static if_op_t if_str2op(char *s)
{
    if (!strcmp(s, "=")) {
        return IF_OP_EQ;
    } else if (!strcmp(s, "!=")) {
        return IF_OP_NE;
    } else if (!strcmp(s, ">")) {
        return IF_OP_GT;
    } else if (!strcmp(s, "<")) {
        return IF_OP_LT;
    } else if (!strcmp(s, ">=")) {
        return IF_OP_GE;
    } else if (!strcmp(s, "<=")) {
        return IF_OP_LE;
    } else {
        return IF_OP_INVALID;
    }
}

static int if_elem_true(if_elem_t *self, dataset_t *ds)
{
    char *left = srender_by(self->left, ds);
    char *right = srender_by(self->right, ds);
    int both_num = str_isdecimal(left) && str_isdecimal(right);
    
    switch (self->op) {
    case IF_OP_EQ:
        if (both_num) {
            return atof(left) == atof(right);
        } else {
            return strcmp(left, right) == 0;
        }
    case IF_OP_NE:
        if (both_num) {
            return atof(left) != atof(right);
        } else {
            return strcmp(left, right) != 0;
        }
    case IF_OP_GT:
        if (both_num) {
            return atof(left) > atof(right);
        } else {
            return strcmp(left, right) > 0;
        }
    case IF_OP_LT:
        if (both_num) {
            return atof(left) < atof(right);
        } else {
            return strcmp(left, right) < 0;
        }
    case IF_OP_GE:
        if (both_num) {
            return atof(left) >= atof(right);
        } else {
            return strcmp(left, right) >= 0;
        }
    case IF_OP_LE:
        if (both_num) {
            return atof(left) <= atof(right);
        } else {
            return strcmp(left, right) <= 0;
        }
    default:
        return 0;
    }
}

static char *render_if_elem(void *elem, dataset_t *ds)
{
    if_elem_t *ie = (if_elem_t *)elem;
    if (if_elem_true(ie, ds)) {
        return render_normal_elem(ie->yes, ds);
    } else {
        if (ie->no) {
            return render_normal_elem(ie->no, ds);
        } else {
            return NULL;
        }
    }
}

static void _render_each_item(int leaf, void *value, void *args)
{
    void **params = (void **)args;
    for_elem_t *fe = params[0];
    dataset_t *ds = params[1];
    char **presult = params[2];
    char *item_name = params[3];
    void *tmp;

    if (leaf) {
        ds->set(ds, item_name, value);
    } else {
        ds->sub.children.set_sp(&ds->sub.children, item_name, value);
    }
    
    // render
    tmp = render_normal_elem(fe->body, ds);
    str_append(presult, tmp);
    free(tmp);
}

static char *render_for_elem(void *elem, dataset_t *ds)
{
    for_elem_t *fe = (for_elem_t *)elem;
    char *result = NULL, item_name[TMPL_NAME_LEN] = FOR_ITEM_NAME_PREFIX;
    void *params[] = {fe, ds, &result, item_name};

    STR_APPEND(item_name, sizeof(item_name), "%s", fe->item);
    ds->foreach_by(ds, fe->list, _render_each_item, params);
    ds->sub.children.set_sp(&ds->sub.children, item_name, NULL);
    
    return result;
}

static char *render_normal_elem(void *elem, dataset_t *ds)
{
    normal_elem_t *self = (normal_elem_t *)elem;
    base_elem_t *be;
    char *result = NULL, *text, *item, *value, *tmp;
    char name[TMPL_NAME_LEN + 1], for_name[TMPL_NAME_LEN + 1], end[2];
    char fmt1[TMPL_FMT_LEN + 1], fmt2[TMPL_FMT_LEN + 1];
    int ret, first_is_var, first = 1;
    u32 nlen;

    if (!list_empty(&self->children)) {
        list_for_each_entry(be, &self->children, link) {
            tmp = be->render(be, ds);
            str_append(&result, tmp);
            FREE_IF_NOT_NULL(tmp);
        }
        return result;
    } else {
        text = strdup(self->text);
        if (!text) {
            return NULL;
        }

        first_is_var = text[0] == '@';
        snprintf(fmt1, sizeof fmt1, "%%%d[a-zA-Z0-9_.]", TMPL_NAME_LEN);
        snprintf(fmt2, sizeof fmt2, "{%%%d[^}]%%%d[\\}]", TMPL_NAME_LEN - 2, 1);

        foreach_item_in_str(item, text, "@") {
            if (first && !first_is_var) {
                first = 0;
                str_append(&result, item);
                continue;
            }
            
            ret = sscanf(item, fmt1, name) == 1;
            if (ret) {
                nlen = strlen(name);// @xxx
            } else {
                ret = sscanf(item, fmt2, name, end) == 2;
                nlen = strlen(name) + 2; // @{xxx}
            }

            snprintf(for_name, sizeof for_name, "%s%s", FOR_ITEM_NAME_PREFIX, name);
            
            if (ret) {// render
                value = ds->get(ds, for_name);
                if (!value) {
                    value = ds->get(ds, name);
                }

                if (value) {
                    str_append(&result, value);
                    str_append(&result, item + nlen);
                    continue;
                }
            }

            // unmatched
            str_append(&result, "@");
            str_append(&result, item);
        }
        
        free(text);

        return result;
    }
}

static void parse_indent(char **p_text, u32 *p_text_len, char **p_indent, int *p_indent_len)
{
    int i;

    if (!*p_text_len) {
        *p_indent = *p_text;
        *p_indent_len = 0;
        return;
    }
    
    if ((*p_text)[0] == '\n') {
        ++*p_text;
        --*p_text_len;
    }
    
    *p_indent = *p_text;
    
    for (i = 0; i < *p_text_len; ++i) {
        if ((*p_text)[i] != ' ' && (*p_text)[i] != '\t') {
            break;
        }
    }
    
    *p_text_len -= i;
    *p_text += i;
    *p_indent_len = i;
}

static int parse_if_elem(char *text, u32 len, char **pnext, if_elem_t **pelem)
{
    char left[TMPL_NAME_LEN + 1], right[TMPL_NAME_LEN + 1], op_str[TMPL_OP_LEN + 1];
    char fmt1[TMPL_FMT_LEN + 1], fmt2[TMPL_FMT_LEN + 1];
    char *indent;
    char *ptrue, *pfalse, *end, *tmp;
    int ret, tlen, flen, dlen, indent_len;
    if_elem_t *self;
    if_op_t op;

    parse_indent(&text, &len, &indent, &indent_len);
    
    // "#if <var> <op> <val>:"

    snprintf(fmt1, sizeof fmt1,
        "#if %%%d[@a-zA-Z0-9.] %%%d[><=!] %%%d[@a-zA-Z0-9.]:",
        TMPL_NAME_LEN, TMPL_OP_LEN, TMPL_NAME_LEN);

    ret = sscanf(text, fmt1, left, op_str, right) == 3;
    if (!ret) {
        return 0;
    }
    
    op = if_str2op(op_str);
    if (op == IF_OP_INVALID) {
        return 0;
    }

    if (!(ptrue = strchr(text, ':')) || !(ptrue = strchr(ptrue, '\n'))) {
        return 0;
    }

    // "#end"
    
    snprintf(fmt1, sizeof fmt1, "\n%.*s#end\r\n", indent_len, indent);
    snprintf(fmt2, sizeof fmt2, "\n%.*s#end\n", indent_len, indent);
    if ((end = strnstr(ptrue, fmt1, text + len - ptrue))) {
        tmp = end + strlen(fmt1);
    } else if ((end = strnstr(ptrue, fmt2, text + len - ptrue))) {
        tmp = end + strlen(fmt2);
    } else {
        return 0;
    }
    *pnext = tmp;

    // "#else:"

    snprintf(fmt1, sizeof fmt1, "\n%.*s#else:", indent_len, indent);
    pfalse = strnstr(ptrue, fmt1, end - ptrue);
    if (pfalse) {
        tlen = pfalse - ptrue + 1;
        pfalse += strlen(fmt1);
        flen = end - pfalse + 1;// +1: include \n
    } else {
        pfalse = NULL;
        tlen = end - ptrue + 1;// +1: include \n
    }

    // parseing ... 

    dlen = strlen(left)+1 + strlen(right)+1;
    *pelem = self = (if_elem_t *)malloc(sizeof *self + dlen);
    if (!self) {
        MEMFAIL();
        return 0;
    }

    memset(self, 0, sizeof *self + dlen);
    self->left = self->data;
    self->op = op;
    strcpy(self->left, left);
    self->right = self->left + strlen(self->left)+1;
    strcpy(self->right, right);
    self->base.render = render_if_elem;

    if (!parse_normal_elem(ptrue, tlen, &tmp, &self->yes)) {
        free(self);
        return 0;
    }

    if (pfalse) {
        if (!parse_normal_elem(pfalse, flen, &tmp, &self->no)) {
            free(self);
            return 0;
        }
    }

    return 1;
}

static int parse_for_elem(char *text, u32 len, char **pnext, for_elem_t **pelem)
{
    char item[TMPL_NAME_LEN + 1], list[TMPL_NAME_LEN + 1];
    char fmt1[TMPL_FMT_LEN + 1], fmt2[TMPL_FMT_LEN + 1];
    char *pbody, *end, *tmp, *indent;
    int ret, blen, dlen, indent_len;
    for_elem_t *self;
    
    parse_indent(&text, &len, &indent, &indent_len);

    // "#for <item> in <list>:"

    snprintf(fmt1, sizeof fmt1, "#for @%%%d[0-9a-zA-Z] in @%%%d[0-9a-zA-Z.]:",
        TMPL_NAME_LEN, TMPL_NAME_LEN);

    ret = sscanf(text, fmt1, item, list);
    if (ret != 2) {
        return 0;
    }

    // "#end"
    snprintf(fmt1, sizeof fmt1, "\n%.*s#end\r\n", indent_len, indent);
    snprintf(fmt2, sizeof fmt2, "\n%.*s#end\n", indent_len, indent);
    if ((end = strnstr(text, fmt1, len))) {
        tmp = end + strlen(fmt1);
    } else if ((end = strnstr(text, fmt2, len))) {
        tmp = end + strlen(fmt2);
    } else {
        return 0;
    }
    *pnext = tmp;

    // body
    if (!(pbody = strchr(text, ':')) || !(pbody = strchr(pbody, '\n'))) {
        return 0;
    }
    blen = end - pbody + 1;

    // parseing ... item, list, body

    dlen = strlen(item)+1 + strlen(list)+1;
    *pelem = self = (for_elem_t *)malloc(sizeof *self + dlen);
    if (!self) {
        MEMFAIL();
        return 0;
    }
    
    memset(self, 0, sizeof *self + dlen);
    self->item = self->data;
    strcpy(self->item, item);
    self->list = self->item + strlen(self->item)+1;
    strcpy(self->list, list);
    self->base.render = render_for_elem;

    if (!parse_normal_elem(pbody, blen, &tmp, &self->body)) {
        free(self);
        return 0;
    }

    return 1;
}

static int parse_normal_elem(char *text, u32 len, char **pnext, normal_elem_t **pelem)
{
    char *prev, *next, *tmp, *pos, *end;
    u32 prev_len = 0;
    normal_elem_t *self, *ne;
    if_elem_t *ie;
    for_elem_t *fe;

    // clone text
    tmp = (char *)malloc(len + 1);
    if (!tmp) {
        MEMFAIL();
        return 0;
    }
    memcpy(tmp, text, len);
    tmp[len] = 0;
    text = tmp;
    end = text + len;

    // new element
    *pelem = self = (normal_elem_t *)malloc(sizeof *self);
    if (!self) {
        MEMFAIL();
        free(text);
        return 0;
    }
    
    memset(self, 0, sizeof *self);
    INIT_LIST_HEAD(&self->children);
    self->base.render = render_normal_elem;

    // parse text ...
    for (prev = pos = text; pos < end; pos = next) {
        if (parse_if_elem(pos, end - pos, &next, &ie)) {
            prev_len = pos - prev;
            if (prev_len) {
                // add prev to children
                parse_normal_elem(prev, prev_len, &tmp, &ne);
                list_add_tail(&ne->base.link, &self->children);
            }
            // add if to childred
            list_add_tail(&ie->base.link, &self->children);
            prev = next;
        } else if (parse_for_elem(pos, end - pos, &next, &fe)) {
            prev_len = pos - prev;
            if (prev_len) {
                // add prev to children
                parse_normal_elem(prev, prev_len, &tmp, &ne);
                list_add_tail(&ne->base.link, &self->children);
            }
            // add for to childred
            list_add_tail(&fe->base.link, &self->children);
            prev = next;
        } else {
            next = strstr(pos+1, "\n");
            if (!next) {
                break;
            }
        }
    }

    // the rest
    if (list_empty(&self->children)) {
        self->text = text;
    } else {
        // add prev to children
        prev_len = end - prev;
        if (prev_len) {
            // add prev to children
            parse_normal_elem(prev, prev_len, &next, &ne);
            list_add_tail(&ne->base.link, &self->children);
        }
        //
        free(text);
    }

    return 1;
}

static template_t *get_template(char *text)
{
    template_t *t;
    char *tmp;
    
    if (!text) {
        return NULL;
    }

    if (!dict_init(&cache)) {
        return NULL;
    }
    
    t = (template_t *)cache.get_sp(&cache, text);
    if (t) {
        return t;
    }

    if (!parse_normal_elem(text, strlen(text) + 1, &tmp, (normal_elem_t **)&t)) {
        return NULL;
    }

    cache.set_sp(&cache, text, t);

    return t;
}

char *srender_by(char *s, dataset_t *ds)
{
    template_t *root;
    char *result;

    if (!s || !ds) {
        return "";
    }

    root = get_template(s);
    if (!root) {
        return "";
    }
    
    result = render_normal_elem((normal_elem_t *)root, ds);
    
    if (result) {
        FREE_LATER(result);
        return result;
    } else {
        return "";
    }
}

char *frender_by(char *filename, dataset_t *ds)
{
    char *s, *result;

    if (!filename || !ds) {
        return "";
    }

    if (!get_file(filename, (void **)&s, NULL)) {
        return "";
    }

    result = srender_by(s, ds);
    free(s);
    return result;
}

static void args2ds(dataset_t *ds, char *fmt, va_list ap)
{
    char *pair, key[50], val[100], values[1024];
    int ret;

    fmt = strdup(fmt);
    if (!fmt) {
        return;
    }
    
    dataset_init(ds);

    replace_char(fmt, ',', 1);
    vsnprintf(values, sizeof values, fmt, ap);

    // fmt = "key=val,xxx=xxx"
    foreach_item_in_str(pair, values, "\x01") {
        ret = sscanf(pair, "%49[^=]=%99[^=]", key, val);
        if (ret != 2) {
            continue;
        }
        ds->set(ds, key, val);
    }

    free(fmt);
}

char *vsrender(char *s, char *fmt, va_list ap)
{
    dataset_t ds = {.inited=0};

    if (!s || !fmt) {
        return "";
    }

    args2ds(&ds, fmt, ap);

    return srender_by(s, &ds);
}

char *srender(char *s, char *fmt, ...)
{
    va_list ap;
    char *ret;

    if (!s || !fmt) {
        return "";
    }

    va_start(ap, fmt);
    ret = vsrender(s, fmt, ap);
    va_end(ap);
    return ret;
}

char *vfrender(char *f, char *fmt, va_list ap)
{
    dataset_t ds = {.inited=0};

    if (!f || !fmt) {
        return "";
    }

    args2ds(&ds, fmt, ap);

    return frender_by(f, &ds);
}

char *frender(char *f, char *fmt, ...)
{
    va_list ap;
    char *ret;

    if (!f || !fmt) {
        return "";
    }

    va_start(ap, fmt);
    ret = vfrender(f, fmt, ap);
    va_end(ap);
    return ret;
}

