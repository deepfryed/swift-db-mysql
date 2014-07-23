// vim:ts=4:sts=4:sw=4:expandtab

// (c) Bharanee Rathna 2012

#include "common.h"
#include "typecast.h"

#ifdef HAVE_CONST_UUID_VARIANT_NCS
    #include <uuid/uuid.h>
#else
    // TODO: no guarantee of being unique.
    typedef unsigned char uuid_t[16];
    void uuid_generate(uuid_t uuid) {
        for (int i = 0; i < sizeof(uuid); i++)
            uuid[i] = rand() % 256;
    }
#endif

VALUE db_mysql_adapter_escape(VALUE, VALUE);

VALUE rb_uuid_string() {
    size_t n;
    uuid_t uuid;
    char uuid_hex[sizeof(uuid_t) * 2 + 1];

    uuid_generate(uuid);
    for (n = 0; n < sizeof(uuid_t); n++)
        sprintf(uuid_hex + n * 2 + 1, "%02x", uuid[n]);

    uuid_hex[0] = 'u';
    return rb_str_new(uuid_hex, sizeof(uuid_t) * 2 + 1);
}

size_t db_mysql_buffer_adjust(char **buffer, size_t size, size_t offset, size_t need) {
    if (need > size - offset)
        *buffer = realloc(*buffer, size += (need > 4096 ? need + 4096 : 4096));
    return size;
}

/* NOTE: very naive, no regex etc. */
VALUE db_mysql_bind_sql(VALUE adapter, VALUE sql, VALUE bind) {
    VALUE value;
    size_t size = 4096;
    char *ptr, *buffer;
    size_t i = 0, j = 0, n = 0;

    buffer = (char *)malloc(size);
    ptr    = RSTRING_PTR(sql);

    while (i < (size_t)RSTRING_LEN(sql)) {
        if (*ptr == '?') {
            if (n < (size_t)RARRAY_LEN(bind)) {
                value = rb_ary_entry(bind, n++);
                if (NIL_P(value)) {
                    size = db_mysql_buffer_adjust(&buffer, size, j, 4);
                    j   += sprintf(buffer + j, "NULL");
                }
                else {
                    value = db_mysql_adapter_escape(adapter, typecast_to_string(value));
                    size  = db_mysql_buffer_adjust(&buffer, size, j, RSTRING_LEN(value) + 2);
                    j    += sprintf(buffer + j, "'%s'", RSTRING_PTR(value));
                }
            }
            else {
                buffer[j++] = *ptr;
                n++;
            }
        }
        else {
            buffer[j++] = *ptr;
        }

        i++;
        ptr++;

        if (j >= size)
            buffer = realloc(buffer, size += 4096);
    }

    sql = rb_str_new(buffer, j);
    free(buffer);

    if (n != (size_t)RARRAY_LEN(bind))
        rb_raise(eSwiftArgumentError, "expected %d bind arguments got %d instead", n, RARRAY_LEN(bind));
    return sql;
}
