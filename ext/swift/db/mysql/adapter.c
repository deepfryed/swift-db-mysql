// vim:ts=4:sts=4:sw=4:expandtab

// (c) Bharanee Rathna 2012

#include "adapter.h"
#include "typecast.h"

/* declaration */
VALUE cDMA, sUser;
VALUE db_mysql_result_each(VALUE);
VALUE db_mysql_result_allocate(VALUE);
VALUE db_mysql_result_load(VALUE, MYSQL_RES *, size_t, size_t);
VALUE db_mysql_statement_allocate(VALUE);
VALUE db_mysql_statement_initialize(VALUE, VALUE, VALUE);

/* definition */
Adapter* db_mysql_adapter_handle(VALUE self) {
    Adapter *a;
    Data_Get_Struct(self, Adapter, a);
    if (!a)
        rb_raise(eSwiftRuntimeError, "Invalid mysql adapter");
    return a;
}

Adapter* db_mysql_adapter_handle_safe(VALUE self) {
    Adapter *a = db_mysql_adapter_handle(self);
    if (!a->connection)
        rb_raise(eSwiftConnectionError, "mysql database is not open");
    return a;
}

void db_mysql_adapter_mark(Adapter *a) {
    if (a)
        rb_gc_mark_maybe(a->io);
}

VALUE db_mysql_adapter_deallocate(Adapter *a) {
    if (a && a->connection)
        mysql_close(a->connection);
    if (a)
        free(a);
}

VALUE db_mysql_adapter_allocate(VALUE klass) {
    Adapter *a = (Adapter*)malloc(sizeof(Adapter));

    a->connection = 0;
    a->t_nesting  = 0;
    a->io         = Qnil;
    return Data_Wrap_Struct(klass, db_mysql_adapter_mark, db_mysql_adapter_deallocate, a);
}

int db_mysql_adapter_infile_init(void **ptr, const char *filename, void *self) {
    Adapter *a = db_mysql_adapter_handle_safe((VALUE)self);
    *ptr = (void *)self;
    return NIL_P(a->io) ? -1 : 0;
}

int db_mysql_adapter_infile_read(void *ptr, char *buffer, unsigned int size) {
    VALUE data;
    Adapter *a = db_mysql_adapter_handle_safe((VALUE)ptr);

    data = rb_funcall(a->io, rb_intern("read"), 1, INT2NUM(size));

    if (NIL_P(data)) return 0;

    memcpy(buffer, RSTRING_PTR(data), RSTRING_LEN(data));
    return RSTRING_LEN(data);
}

void db_mysql_adapter_infile_end(void *ptr) {
    Adapter *a = db_mysql_adapter_handle_safe((VALUE)ptr);
    a->io = Qnil;
}

int db_mysql_adapter_infile_error(void *ptr, char *error, unsigned int size) {
    Adapter *a = db_mysql_adapter_handle_safe((VALUE)ptr);
    a->io = Qnil;
    snprintf(error, size, "error loading data using LOAD INFILE");
    return 0;
}

VALUE db_mysql_adapter_initialize(VALUE self, VALUE options) {
    char MYSQL_BOOL_TRUE = 1;
    VALUE db, user, pass, host, port;
    Adapter *a = db_mysql_adapter_handle(self);

    if (TYPE(options) != T_HASH)
        rb_raise(eSwiftArgumentError, "options needs to be a hash");

    db   = rb_hash_aref(options, ID2SYM(rb_intern("db")));
    user = rb_hash_aref(options, ID2SYM(rb_intern("user")));
    pass = rb_hash_aref(options, ID2SYM(rb_intern("pass")));
    host = rb_hash_aref(options, ID2SYM(rb_intern("host")));
    port = rb_hash_aref(options, ID2SYM(rb_intern("port")));

    if (NIL_P(db))
        rb_raise(eSwiftConnectionError, "Invalid db name");
    if (NIL_P(host))
        host = rb_str_new2("127.0.0.1");
    if (NIL_P(port))
        port = rb_str_new2("3306");
    if (NIL_P(user))
        user = sUser;

    a->connection = mysql_init(0);
    mysql_options(a->connection, MYSQL_OPT_RECONNECT, &MYSQL_BOOL_TRUE);
    mysql_options(a->connection, MYSQL_OPT_LOCAL_INFILE, 0);

    if (!mysql_real_connect(a->connection,
        CSTRING(host), CSTRING(user), CSTRING(pass), CSTRING(db), atoi(CSTRING(port)), 0, CLIENT_FOUND_ROWS))
        rb_raise(eSwiftConnectionError, "%s", mysql_error(a->connection));

    mysql_set_character_set(a->connection, "utf8");
    mysql_set_local_infile_handler(
        a->connection,
        db_mysql_adapter_infile_init, db_mysql_adapter_infile_read, db_mysql_adapter_infile_end, db_mysql_adapter_infile_error,
        (void *)self
    );
    return self;
}

typedef struct Query {
    MYSQL *connection;
    VALUE sql;
} Query;

VALUE nogvl_mysql_adapter_execute(void *ptr) {
    Query *q = (Query *)ptr;
    return (VALUE)mysql_real_query(q->connection, RSTRING_PTR(q->sql), RSTRING_LEN(q->sql));
}

VALUE db_mysql_adapter_execute(int argc, VALUE *argv, VALUE self) {
    VALUE sql, bind;
    MYSQL_RES *result;
    Adapter *a = db_mysql_adapter_handle_safe(self);
    MYSQL *c   = a->connection;

    rb_scan_args(argc, argv, "10*", &sql, &bind);
    sql = TO_S(sql);

    if (RARRAY_LEN(bind) > 0)
        sql = db_mysql_bind_sql(self, sql, bind);

    Query q = {.connection = c, .sql = sql};

    if ((int)rb_thread_blocking_region(nogvl_mysql_adapter_execute, &q, RUBY_UBF_IO, 0) != 0)
        rb_raise(eSwiftRuntimeError, "%s", mysql_error(c));

    result = mysql_store_result(c);
    return db_mysql_result_load(db_mysql_result_allocate(cDMR), result, mysql_insert_id(c), mysql_affected_rows(c));
}

VALUE db_mysql_adapter_begin(int argc, VALUE *argv, VALUE self) {
    char command[256];
    VALUE savepoint;

    Adapter *a = db_mysql_adapter_handle_safe(self);
    rb_scan_args(argc, argv, "01", &savepoint);

    if (a->t_nesting == 0) {
        strcpy(command, "begin");
        if (mysql_real_query(a->connection, command, strlen(command)) != 0)
            rb_raise(eSwiftRuntimeError, "%s", mysql_error(a->connection));
        a->t_nesting++;
        if (NIL_P(savepoint))
            return Qtrue;
    }

    if (NIL_P(savepoint))
        savepoint = rb_uuid_string();

    snprintf(command, 256, "savepoint %s", CSTRING(savepoint));
    if (mysql_real_query(a->connection, command, strlen(command)) != 0)
        rb_raise(eSwiftRuntimeError, "%s", mysql_error(a->connection));

    a->t_nesting++;
    return savepoint;
}

VALUE db_mysql_adapter_commit(int argc, VALUE *argv, VALUE self) {
    VALUE savepoint;
    char command[256];

    Adapter *a = db_mysql_adapter_handle_safe(self);
    rb_scan_args(argc, argv, "01", &savepoint);

    if (a->t_nesting == 0)
        return Qfalse;

    if (NIL_P(savepoint)) {
        strcpy(command, "commit");
        if (mysql_real_query(a->connection, command, strlen(command)) != 0)
            rb_raise(eSwiftRuntimeError, "%s", mysql_error(a->connection));
        a->t_nesting--;
    }
    else {
        snprintf(command, 256, "release savepoint %s", CSTRING(savepoint));
        if (mysql_real_query(a->connection, command, strlen(command)) != 0)
            rb_raise(eSwiftRuntimeError, "%s", mysql_error(a->connection));
        a->t_nesting--;
    }
    return Qtrue;
}

VALUE db_mysql_adapter_rollback(int argc, VALUE *argv, VALUE self) {
    VALUE savepoint;
    char command[256];

    Adapter *a = db_mysql_adapter_handle_safe(self);
    rb_scan_args(argc, argv, "01", &savepoint);

    if (a->t_nesting == 0)
        return Qfalse;

    if (NIL_P(savepoint)) {
        strcpy(command, "rollback");
        if (mysql_real_query(a->connection, command, strlen(command)) != 0)
            rb_raise(eSwiftRuntimeError, "%s", mysql_error(a->connection));
        a->t_nesting--;
    }
    else {
        snprintf(command, 256, "rollback to savepoint %s", CSTRING(savepoint));
        if (mysql_real_query(a->connection, command, strlen(command)) != 0)
            rb_raise(eSwiftRuntimeError, "%s", mysql_error(a->connection));
        a->t_nesting--;
    }
    return Qtrue;
}

VALUE db_mysql_adapter_transaction(int argc, VALUE *argv, VALUE self) {
    int status;
    VALUE savepoint, block, block_result;

    Adapter *a = db_mysql_adapter_handle_safe(self);
    rb_scan_args(argc, argv, "01&", &savepoint, &block);

    if (NIL_P(block))
        rb_raise(eSwiftRuntimeError, "mysql transaction requires a block");

    if (a->t_nesting == 0) {
        db_mysql_adapter_begin(1, &savepoint, self);
        block_result = rb_protect(rb_yield, self, &status);
        if (!status) {
            db_mysql_adapter_commit(1, &savepoint, self);
            if (!NIL_P(savepoint))
                db_mysql_adapter_commit(0, 0, self);
        }
        else {
            db_mysql_adapter_rollback(1, &savepoint, self);
            if (!NIL_P(savepoint))
                db_mysql_adapter_rollback(0, 0, self);
            rb_jump_tag(status);
        }
    }
    else {
        if (NIL_P(savepoint))
            savepoint = rb_uuid_string();
        db_mysql_adapter_begin(1, &savepoint, self);
        block_result = rb_protect(rb_yield, self, &status);
        if (!status)
            db_mysql_adapter_commit(1, &savepoint, self);
        else {
            db_mysql_adapter_rollback(1, &savepoint, self);
            rb_jump_tag(status);
        }
    }

    return block_result;
}

VALUE db_mysql_adapter_close(VALUE self) {
    Adapter *a = db_mysql_adapter_handle(self);
    if (a->connection) {
        mysql_close(a->connection);
        a->connection = 0;
        return Qtrue;
    }
    return Qfalse;
}

VALUE db_mysql_adapter_closed_q(VALUE self) {
    Adapter *a = db_mysql_adapter_handle(self);
    return a->connection ? Qfalse : Qtrue;
}

VALUE db_mysql_adapter_prepare(VALUE self, VALUE sql) {
    return db_mysql_statement_initialize(db_mysql_statement_allocate(cDMS), self, sql);
}

VALUE db_mysql_adapter_escape(VALUE self, VALUE fragment) {
    VALUE text = TO_S(fragment);
    char escaped[RSTRING_LEN(text) * 2 + 1];
    Adapter *a = db_mysql_adapter_handle_safe(self);
    mysql_real_escape_string(a->connection, escaped, RSTRING_PTR(text), RSTRING_LEN(text));
    return rb_str_new2(escaped);
}

VALUE db_mysql_adapter_fileno(VALUE self) {
    Adapter *a = db_mysql_adapter_handle_safe(self);
    return INT2NUM(a->connection->net.fd);
}

VALUE db_mysql_adapter_query(int argc, VALUE *argv, VALUE self) {
    VALUE sql, bind, result;
    MYSQL_RES *r;
    Adapter *a = db_mysql_adapter_handle_safe(self);
    MYSQL *c   = a->connection;

    rb_scan_args(argc, argv, "10*", &sql, &bind);
    sql = TO_S(sql);

    if (RARRAY_LEN(bind) > 0)
        sql = db_mysql_bind_sql(self, sql, bind);

    mysql_send_query(c, RSTRING_PTR(sql), RSTRING_LEN(sql));

    if (rb_block_given_p()) {
        rb_thread_wait_fd(a->connection->net.fd);
        if (mysql_read_query_result(c) != 0)
            rb_raise(eSwiftRuntimeError, "%s", mysql_error(c));

        r      = mysql_store_result(c);
        result = db_mysql_result_load(db_mysql_result_allocate(cDMR), r, mysql_insert_id(c), mysql_affected_rows(c));
        return db_mysql_result_each(result);
    }

    return Qtrue;
}

VALUE db_mysql_adapter_result(VALUE self) {
    MYSQL_RES *r;
    Adapter *a = db_mysql_adapter_handle_safe(self);
    MYSQL *c   = a->connection;

    if (mysql_read_query_result(c) != 0)
        rb_raise(eSwiftRuntimeError, "%s", mysql_error(c));

    r = mysql_store_result(c);
    return db_mysql_result_load(db_mysql_result_allocate(cDMR), r, mysql_insert_id(c), mysql_affected_rows(c));
}

VALUE db_mysql_adapter_write(int argc, VALUE *argv, VALUE self) {
    VALUE table, fields, io, data;

    char *sql;
    Adapter *a = db_mysql_adapter_handle_safe(self);
    MYSQL   *c = a->connection;

    if (argc < 2 || argc > 3)
        rb_raise(rb_eArgError, "wrong number of arguments (%d for 2..3)", argc);

    table = fields = io = Qnil;
    switch (argc) {
        case 2:
            table = argv[0];
            io    = argv[1];
            break;
        case 3:
            table  = argv[0];
            fields = argv[1];
            io     = argv[2];
            if (TYPE(fields) != T_ARRAY)
                rb_raise(eSwiftArgumentError, "fields needs to be an array");
            if (RARRAY_LEN(fields) < 1)
                fields = Qnil;
    }

    if (argc > 1) {
        sql = (char *)malloc(4096);
        if (NIL_P(fields))
            snprintf(sql, 4096, "load data local infile 'swift' replace into table %s", CSTRING(table));
        else
            snprintf(sql, 4096, "load data local infile 'swift' replace into table %s(%s)",
                CSTRING(table), CSTRING(rb_ary_join(fields, rb_str_new2(", "))));

        a->io = rb_respond_to(io, rb_intern("read")) ? io : rb_funcall(cStringIO, rb_intern("new"), 1, TO_S(io));
        rb_gc_mark(a->io);
        if (mysql_real_query(a->connection, sql, strlen(sql)) != 0) {
            free(sql);
            a->io = Qnil;
            rb_raise(eSwiftRuntimeError, "%s", mysql_error(a->connection));
        }

        free(sql);
    }

    return db_mysql_result_load(db_mysql_result_allocate(cDMR), 0, mysql_insert_id(c), mysql_affected_rows(c));
}

void init_swift_db_mysql_adapter() {
    rb_require("etc");
    sUser  = rb_funcall(CONST_GET(rb_mKernel, "Etc"), rb_intern("getlogin"), 0);
    cDMA   = rb_define_class_under(mDB, "Mysql", rb_cObject);

    rb_define_alloc_func(cDMA, db_mysql_adapter_allocate);

    rb_define_method(cDMA, "initialize",  db_mysql_adapter_initialize,   1);
    rb_define_method(cDMA, "execute",     db_mysql_adapter_execute,     -1);
    rb_define_method(cDMA, "prepare",     db_mysql_adapter_prepare,      1);
    rb_define_method(cDMA, "begin",       db_mysql_adapter_begin,       -1);
    rb_define_method(cDMA, "commit",      db_mysql_adapter_commit,      -1);
    rb_define_method(cDMA, "rollback",    db_mysql_adapter_rollback,    -1);
    rb_define_method(cDMA, "transaction", db_mysql_adapter_transaction, -1);
    rb_define_method(cDMA, "close",       db_mysql_adapter_close,        0);
    rb_define_method(cDMA, "closed?",     db_mysql_adapter_closed_q,     0);
    rb_define_method(cDMA, "escape",      db_mysql_adapter_escape,       1);
    rb_define_method(cDMA, "fileno",      db_mysql_adapter_fileno,       0);
    rb_define_method(cDMA, "query",       db_mysql_adapter_query,       -1);
    rb_define_method(cDMA, "result",      db_mysql_adapter_result,       0);
    rb_define_method(cDMA, "write",       db_mysql_adapter_write,       -1);

    rb_global_variable(&sUser);
}
