// vim:ts=4:sts=4:sw=4:expandtab

// (c) Bharanee Rathna 2012

#include "statement.h"
#include "adapter.h"
#include "typecast.h"

/* declaration */

VALUE cDMS;

VALUE    db_mysql_result_allocate(VALUE);
VALUE    db_mysql_result_from_statement(VALUE, VALUE);
Adapter* db_mysql_adapter_handle_safe(VALUE);

/* definition */

Statement* db_mysql_statement_handle(VALUE self) {
    Statement *s;
    Data_Get_Struct(self, Statement, s);
    if (!s)
        rb_raise(eSwiftRuntimeError, "Invalid mysql statement");
    return s;
}

Statement* db_mysql_statement_handle_safe(VALUE self) {
    Statement *s = db_mysql_statement_handle(self);
    if (!s->statement)
        rb_raise(eSwiftRuntimeError, "statement already closed or released");
    return s;
}

void db_mysql_statement_mark(Statement *s) {
    if (s && s->adapter)
        rb_gc_mark_maybe(s->adapter);
}

VALUE db_mysql_statement_deallocate(Statement *s) {
    if (s && s->statement) {
            mysql_stmt_close(s->statement);
        free(s);
    }
}

VALUE db_mysql_statement_allocate(VALUE klass) {
    Statement *s = (Statement*)malloc(sizeof(Statement));
    return Data_Wrap_Struct(klass, db_mysql_statement_mark, db_mysql_statement_deallocate, s);
}

VALUE db_mysql_statement_initialize(VALUE self, VALUE adapter, VALUE sql) {
    int i;
    MYSQL *connection;
    Statement *s = db_mysql_statement_handle(self);

    s->adapter = adapter;
    rb_gc_mark(s->adapter);

    connection   = db_mysql_adapter_handle_safe(adapter)->connection;
    s->statement = mysql_stmt_init(connection);
    sql          = TO_S(sql);

    if (mysql_stmt_prepare(s->statement, RSTRING_PTR(sql), RSTRING_LEN(sql)) != 0)
        rb_raise(eSwiftRuntimeError, "%s", mysql_stmt_error(s->statement));

    return self;
}

VALUE db_mysql_statement_execute(int argc, VALUE *argv, VALUE self) {
    int n, error;
    VALUE bind, data, result;
    MYSQL_BIND *mysql_bind;
    char MYSQL_BOOL_TRUE = 1, MYSQL_BOOL_FALSE = 0;

    Statement *s = db_mysql_statement_handle_safe(self);

    rb_scan_args(argc, argv, "00*", &bind);

    if (RARRAY_LEN(bind) > 0) {
        n = mysql_stmt_param_count(s->statement);
        if (RARRAY_LEN(bind) != n)
            rb_raise(eSwiftArgumentError, "expected %d bind arguments got %d instead", n, RARRAY_LEN(bind));
        mysql_bind = (MYSQL_BIND *)malloc(sizeof(MYSQL_BIND) * RARRAY_LEN(bind));
        bzero(mysql_bind, sizeof(MYSQL_BIND) * RARRAY_LEN(bind));

        for (n = 0; n < RARRAY_LEN(bind); n++) {
            data = rb_ary_entry(bind, n);
            if (NIL_P(data)) {
                mysql_bind[n].is_null     = &MYSQL_BOOL_TRUE;
                mysql_bind[n].buffer_type = MYSQL_TYPE_NULL;
            }
            else {
                data = typecast_to_string(data);
                mysql_bind[n].is_null       = &MYSQL_BOOL_FALSE;
                mysql_bind[n].buffer_type   = MYSQL_TYPE_STRING;
                mysql_bind[n].buffer        = RSTRING_PTR(data);
                mysql_bind[n].buffer_length = RSTRING_LEN(data);
            }
        }

        if (mysql_stmt_bind_param(s->statement, mysql_bind) != 0) {
            free(mysql_bind);
            rb_raise(eSwiftRuntimeError, mysql_stmt_error(s->statement));
        }

        error = mysql_stmt_execute(s->statement);
        free(mysql_bind);
    }
    else {
        error = mysql_stmt_execute(s->statement);
    }

    if (error)
        rb_raise(eSwiftRuntimeError, mysql_stmt_error(s->statement));

    result = db_mysql_result_allocate(cDMR);
    return db_mysql_result_from_statement(result, self);
}

VALUE db_mysql_statement_release(VALUE self) {
    Statement *s = db_mysql_statement_handle(self);
    if (s->statement) {
        mysql_stmt_close(s->statement);
        s->statement = 0;
        return Qtrue;
    }
    return Qfalse;
}

void init_swift_db_mysql_statement() {
    cDMS = rb_define_class_under(cDMA, "Statement", rb_cObject);
    rb_define_alloc_func(cDMS, db_mysql_statement_allocate);
    rb_define_method(cDMS, "initialize", db_mysql_statement_initialize, 2);
    rb_define_method(cDMS, "execute",    db_mysql_statement_execute,   -1);
    rb_define_method(cDMS, "release",    db_mysql_statement_release,    0);
}
