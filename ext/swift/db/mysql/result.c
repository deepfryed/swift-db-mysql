// vim:ts=4:sts=4:sw=4:expandtab

// (c) Bharanee Rathna 2012

#include "result.h"
#include "statement.h"

#include <stdlib.h>

/* declaration */

typedef struct Result {
    MYSQL_RES *r;
    MYSQL_ROW_OFFSET start;

    MYSQL_BIND *bind;
    unsigned long *lengths;
    my_bool *is_null;

    VALUE fields;
    VALUE types;
    VALUE rows;
    VALUE statement;

    size_t cols;
    size_t selected;
    size_t affected;
    size_t insert_id;
} Result;

VALUE cDMR;
Statement* db_mysql_statement_handle_safe(VALUE);

/* definition */

Result* db_mysql_result_handle(VALUE self) {
    Result *r;
    Data_Get_Struct(self, Result, r);
    if (!r)
        rb_raise(eSwiftRuntimeError, "Invalid mysql result");
    return r;
}

void db_mysql_result_mark(Result *r) {
    if (r) {
        if (!NIL_P(r->fields))
            rb_gc_mark_maybe(r->fields);
        if (!NIL_P(r->types))
            rb_gc_mark_maybe(r->types);
        if (!NIL_P(r->rows))
            rb_gc_mark_maybe(r->rows);
        if (!NIL_P(r->rows))
            rb_gc_mark_maybe(r->statement);
    }
}

VALUE db_mysql_result_deallocate(Result *r) {
    size_t n;
    if (r) {
        if (r->r)
            mysql_free_result(r->r);
        if (r->lengths)
            free(r->lengths);
        if (r->is_null)
            free(r->is_null);
        if (r->bind) {
            for (n = 0; n < r->cols; n++)
                free(r->bind[n].buffer);
            free(r->bind);
        }
        free(r);
    }
}

VALUE db_mysql_result_allocate(VALUE klass) {
    Result *r = (Result*)malloc(sizeof(Result));
    r->statement = Qnil;
    return Data_Wrap_Struct(klass, db_mysql_result_mark, db_mysql_result_deallocate, r);
}

VALUE db_mysql_result_load(VALUE self, MYSQL_RES *result, size_t insert_id, size_t affected) {
    size_t n, rows, cols;
    const char *type, *data;
    MYSQL_FIELD *fields;

    Result *r    = db_mysql_result_handle(self);
    r->fields    = rb_ary_new();
    r->types     = rb_ary_new();
    r->rows      = rb_ary_new();
    r->r         = result;
    r->affected  = affected;
    r->insert_id = insert_id;
    r->selected  = 0;
    r->lengths   = 0;
    r->is_null   = 0;
    r->bind      = 0;
    r->cols      = 0;

    /* non select queries */
    if (!result)
        return self;

    rows   = mysql_num_rows(result);
    cols   = mysql_num_fields(result);
    fields = mysql_fetch_fields(result);

    r->cols     = cols;
    r->selected = rows;

    for (n = 0; n < cols; n++) {
        rb_ary_push(r->fields, ID2SYM(rb_intern(fields[n].name)));

        switch (fields[n].type) {
            case MYSQL_TYPE_TINY:
                rb_ary_push(r->types, (fields[n].length == 1 ? INT2NUM(SWIFT_TYPE_BOOLEAN) : INT2NUM(SWIFT_TYPE_INT)));
                break;
            case MYSQL_TYPE_SHORT:
            case MYSQL_TYPE_LONG:
            case MYSQL_TYPE_INT24:
            case MYSQL_TYPE_LONGLONG:
            case MYSQL_TYPE_YEAR:
                rb_ary_push(r->types, INT2NUM(SWIFT_TYPE_INT));
                break;
            case MYSQL_TYPE_DECIMAL:
            case MYSQL_TYPE_NEWDECIMAL:
                rb_ary_push(r->types, INT2NUM(SWIFT_TYPE_NUMERIC));
                break;
            case MYSQL_TYPE_FLOAT:
            case MYSQL_TYPE_DOUBLE:
                rb_ary_push(r->types, INT2NUM(SWIFT_TYPE_FLOAT));
                break;
            case MYSQL_TYPE_TIMESTAMP:
            case MYSQL_TYPE_DATETIME:
                rb_ary_push(r->types, INT2NUM(SWIFT_TYPE_TIMESTAMP));
                break;
            case MYSQL_TYPE_TIME:
                rb_ary_push(r->types, INT2NUM(SWIFT_TYPE_TIME));
                break;
            case MYSQL_TYPE_DATE:
                rb_ary_push(r->types, INT2NUM(SWIFT_TYPE_DATE));
                break;
            default:
                rb_ary_push(r->types, (fields[n].flags & BINARY_FLAG) ? INT2NUM(SWIFT_TYPE_BLOB) : INT2NUM(SWIFT_TYPE_TEXT));
        }
    }

    return self;
}

VALUE db_mysql_binary_typecast(Result *r, int i) {
    double msec;
    VALUE v;
    MYSQL_TIME *t;

    switch (r->bind[i].buffer_type) {
        case MYSQL_TYPE_TINY:
            if (r->bind[i].is_unsigned)
                v = UINT2NUM(*(unsigned char *)r->bind[i].buffer);
            else
                v = INT2NUM(*(signed char *)r->bind[i].buffer);
            break;
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_YEAR:
            if (r->bind[i].is_unsigned)
                v = UINT2NUM(*(unsigned short *)r->bind[i].buffer);
            else
                v = INT2NUM(*(short *)r->bind[i].buffer);
            break;
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_LONG:
            if (r->bind[i].is_unsigned)
                v = UINT2NUM(*(unsigned int *)r->bind[i].buffer);
            else
                v = INT2NUM(*(int *)r->bind[i].buffer);
            break;
        case MYSQL_TYPE_LONGLONG:
            if (r->bind[i].is_unsigned)
                v = ULL2NUM(*(unsigned long long *)r->bind[i].buffer);
            else
                v = LL2NUM(*(long long *)r->bind[i].buffer);
            break;
        case MYSQL_TYPE_FLOAT:
            v = rb_float_new((double)(*(float *)r->bind[i].buffer));
            break;
        case MYSQL_TYPE_DOUBLE:
            v = rb_float_new(*(double *)r->bind[i].buffer);
            break;
        case MYSQL_TYPE_TIME:
        case MYSQL_TYPE_DATE:
        case MYSQL_TYPE_DATETIME:
        case MYSQL_TYPE_TIMESTAMP:
            t = (MYSQL_TIME *)r->bind[i].buffer;
            v = rb_funcall(cSwiftDateTime, rb_intern("civil"), 7,
                INT2FIX(t->year), INT2FIX(t->month), INT2FIX(t->day),
                INT2FIX(t->hour), INT2FIX(t->minute), INT2FIX(t->second), INT2FIX(0), INT2FIX(0));
            break;
        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_VAR_STRING:
        case MYSQL_TYPE_NEWDECIMAL:
        case MYSQL_TYPE_BIT:
            v = rb_enc_str_new(r->bind[i].buffer, r->lengths[i], rb_utf8_encoding());
            break;
        case MYSQL_TYPE_TINY_BLOB:
        case MYSQL_TYPE_BLOB:
        case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_LONG_BLOB:
            v = rb_funcall(cStringIO, rb_intern("new"), 1, rb_str_new(r->bind[i].buffer, r->lengths[i]));
            break;
        default:
            rb_raise(rb_eTypeError, "unknown buffer_type: %d", r->bind[i].buffer_type);
    }
    return v;
}

VALUE db_mysql_result_from_statement_each(VALUE self) {
    int n;
    size_t row = 0;
    VALUE tuple;
    Result *r;
    MYSQL_STMT *s;

    r = db_mysql_result_handle(self);
    s = db_mysql_statement_handle_safe(r->statement)->statement;

    mysql_stmt_row_seek(s, r->start);

    while (row < r->selected) {
        switch (mysql_stmt_fetch(s)) {
            case MYSQL_NO_DATA:
                break;
            case MYSQL_DATA_TRUNCATED:
                rb_raise(eSwiftRuntimeError, "Bind buffers were under-allocated: MySQL data truncated");
                break;
            case 1:
                rb_raise(eSwiftRuntimeError, "%s", mysql_stmt_error(s));
                break;
            default:
                tuple = rb_hash_new();
                for (n = 0; n < RARRAY_LEN(r->fields); n++) {
                    if (r->is_null[n]) {
                        rb_hash_aset(tuple, rb_ary_entry(r->fields, n), Qnil);
                    }
                    else {
                        rb_hash_aset(tuple, rb_ary_entry(r->fields, n), db_mysql_binary_typecast(r, n));
                    }
                }
                rb_yield(tuple);
        }
        row++;
    }

    return Qtrue;
}

VALUE db_mysql_result_each(VALUE self) {
    MYSQL_STMT *s;
    MYSQL_ROW data;
    size_t *lengths, row, col;
    Result *r = db_mysql_result_handle(self);

    if (!NIL_P(r->statement))
        return db_mysql_result_from_statement_each(self);

    if (!r->r)
        return Qfalse;

    mysql_data_seek(r->r, 0);

    for (row = 0; row < r->selected; row++) {
        VALUE tuple = rb_hash_new();
        data    = mysql_fetch_row(r->r);
        lengths = mysql_fetch_lengths(r->r);

        for (col = 0; col < (size_t)RARRAY_LEN(r->fields); col++) {
            if (!data[col]) {
                rb_hash_aset(tuple, rb_ary_entry(r->fields, col), Qnil);
            }
            else {
                rb_hash_aset(tuple, rb_ary_entry(r->fields, col),
                    typecast_detect(data[col], lengths[col], NUM2INT(rb_ary_entry(r->types, col))));
            }
        }
        rb_yield(tuple);
    }
    return Qtrue;
}

VALUE db_mysql_result_selected_rows(VALUE self) {
    Result *r = db_mysql_result_handle(self);
    return SIZET2NUM(r->selected);
}

VALUE db_mysql_result_affected_rows(VALUE self) {
    Result *r = db_mysql_result_handle(self);
    return SIZET2NUM(r->selected > 0 ? 0 : r->affected);
}

VALUE db_mysql_result_fields(VALUE self) {
    Result *r = db_mysql_result_handle(self);
    return r->fields;
}

VALUE db_mysql_result_types(VALUE self) {
    Result *r = db_mysql_result_handle(self);
    return typecast_description(r->types);
}

VALUE db_mysql_result_insert_id(VALUE self) {
    Result *r = db_mysql_result_handle(self);
    return SIZET2NUM(r->insert_id);
}

VALUE db_mysql_result_from_statement(VALUE self, VALUE statement) {
    int n, row, cols;
    MYSQL_STMT *s;
    MYSQL_FIELD *fields;
    MYSQL_RES *result;
    Result *r = db_mysql_result_handle(self);

    if (!rb_obj_is_kind_of(statement, cDMS))
        rb_raise(eSwiftArgumentError, "invalid Mysql::Statement");

    r->statement = statement;
    s = db_mysql_statement_handle_safe(statement)->statement;

    mysql_stmt_store_result(s);
    result = mysql_stmt_result_metadata(s);
    db_mysql_result_load(self, result, mysql_stmt_insert_id(s), mysql_stmt_affected_rows(s));

    if (result) {
        cols       = mysql_num_fields(result);
        fields     = mysql_fetch_fields(result);
        r->bind    = (MYSQL_BIND *)malloc(sizeof(MYSQL_BIND) * cols);
        r->lengths = (unsigned long *)malloc(sizeof(unsigned long) * cols);
        r->is_null = (my_bool *)malloc(sizeof(my_bool) * cols);
        bzero(r->bind, sizeof(MYSQL_BIND) * cols);

        for (n = 0; n < cols; n++) {
            r->bind[n].length      = &r->lengths[n];
            r->bind[n].is_null     = &r->is_null[n];
            r->bind[n].buffer_type = fields[n].type;

            switch(fields[n].type) {
                case MYSQL_TYPE_NULL:
                    r->bind[n].buffer        = malloc(1);
                    r->bind[n].buffer_length = 1;
                    break;
                case MYSQL_TYPE_TINY:
                case MYSQL_TYPE_SHORT:
                case MYSQL_TYPE_YEAR:
                case MYSQL_TYPE_INT24:
                case MYSQL_TYPE_LONG:
                case MYSQL_TYPE_LONGLONG:
                case MYSQL_TYPE_FLOAT:
                case MYSQL_TYPE_DOUBLE:
                    r->bind[n].buffer        = malloc(8);
                    r->bind[n].buffer_length = 8;
                    bzero(r->bind[n].buffer, 8);
                    break;
                case MYSQL_TYPE_DECIMAL:
                case MYSQL_TYPE_STRING:
                case MYSQL_TYPE_VAR_STRING:
                case MYSQL_TYPE_TINY_BLOB:
                case MYSQL_TYPE_BLOB:
                case MYSQL_TYPE_MEDIUM_BLOB:
                case MYSQL_TYPE_LONG_BLOB:
                case MYSQL_TYPE_NEWDECIMAL:
                case MYSQL_TYPE_BIT:
                    r->bind[n].buffer        = malloc(fields[n].length);
                    r->bind[n].buffer_length = fields[n].length;
                    bzero(r->bind[n].buffer, fields[n].length);
                    if (!(fields[n].flags & BINARY_FLAG))
                         r->bind[n].buffer_type = MYSQL_TYPE_STRING;
                    break;
                case MYSQL_TYPE_TIME:
                case MYSQL_TYPE_DATE:
                case MYSQL_TYPE_DATETIME:
                case MYSQL_TYPE_TIMESTAMP:
                    r->bind[n].buffer        = malloc(sizeof(MYSQL_TIME));
                    r->bind[n].buffer_length = sizeof(MYSQL_TIME);
                    bzero(r->bind[n].buffer, sizeof(MYSQL_TIME));
                    break;
                default:
                    rb_raise(rb_eTypeError, "unknown buffer_type: %d", fields[n].type);
            }
        }

        if (mysql_stmt_bind_result(s, r->bind) != 0)
            rb_raise(eSwiftRuntimeError, "%s", mysql_stmt_error(s));
    }

    r->start    = mysql_stmt_row_tell(s);
    r->selected = mysql_stmt_num_rows(s);
    r->affected = mysql_stmt_affected_rows(s);
    return self;
}

void init_swift_db_mysql_result() {
    rb_require("bigdecimal");
    rb_require("stringio");
    rb_require("date");

    cDMR = rb_define_class_under(cDMA, "Result", rb_cObject);

    rb_include_module(cDMR, rb_mEnumerable);
    rb_define_alloc_func(cDMR, db_mysql_result_allocate);
    rb_define_method(cDMR, "each",          db_mysql_result_each,          0);
    rb_define_method(cDMR, "selected_rows", db_mysql_result_selected_rows, 0);
    rb_define_method(cDMR, "affected_rows", db_mysql_result_affected_rows, 0);
    rb_define_method(cDMR, "fields",        db_mysql_result_fields,        0);
    rb_define_method(cDMR, "types",         db_mysql_result_types,         0);
    rb_define_method(cDMR, "insert_id",     db_mysql_result_insert_id,     0);
}
