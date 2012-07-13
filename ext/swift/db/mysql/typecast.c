// vim:ts=4:sts=4:sw=4:expandtab

// (c) Bharanee Rathna 2012

#include "common.h"
#include "typecast.h"
#include "datetime.h"

#define date_parse(klass, data,len) rb_funcall(datetime_parse(klass, data, len), fto_date, 0)

ID fnew, fto_date, fstrftime;
VALUE cBigDecimal, cStringIO;
VALUE dtformat;
VALUE cDateTime;

VALUE typecast_string(const char *data, size_t n) {
    return rb_enc_str_new(data, n, rb_utf8_encoding());
}

VALUE typecast_detect(const char *data, size_t size, int type) {
    switch (type) {
        case SWIFT_TYPE_INT:
            return rb_cstr2inum(data, 10);
        case SWIFT_TYPE_FLOAT:
            return rb_float_new(atof(data));
        case SWIFT_TYPE_NUMERIC:
            return rb_funcall(cBigDecimal, fnew, 1, rb_str_new(data, size));
        case SWIFT_TYPE_BOOLEAN:
            return (data && (data[0] =='t' || data[0] == '1')) ? Qtrue : Qfalse;
        case SWIFT_TYPE_BLOB:
            return rb_funcall(cStringIO, fnew, 1, rb_str_new(data, size));
        case SWIFT_TYPE_TIMESTAMP:
            return datetime_parse(cSwiftDateTime, data, size);
        case SWIFT_TYPE_DATE:
            return date_parse(cSwiftDateTime, data, size);
        default:
            return rb_enc_str_new(data, size, rb_utf8_encoding());
    }
}

VALUE typecast_to_string(VALUE value) {
    switch (TYPE(value)) {
        case T_STRING:
            return value;
        case T_TRUE:
            return rb_str_new2("1");
        case T_FALSE:
            return rb_str_new2("0");
        default:
            if (rb_obj_is_kind_of(value, rb_cTime) || rb_obj_is_kind_of(value, cDateTime)) {
                return rb_funcall(value, fstrftime, 1, dtformat);
            }
            else if (rb_obj_is_kind_of(value, rb_cIO) || rb_obj_is_kind_of(value, cStringIO)) {
                return rb_funcall(value, rb_intern("read"), 0);
            }
            return rb_funcall(value, rb_intern("to_s"), 0);
    }
}

VALUE typecast_description(VALUE list) {
    int n;
    VALUE types = rb_ary_new();

    for (n = 0; n < RARRAY_LEN(list); n++) {
        switch (NUM2INT(rb_ary_entry(list, n))) {
            case SWIFT_TYPE_INT:
                rb_ary_push(types, rb_str_new2("integer")); break;
            case SWIFT_TYPE_NUMERIC:
                rb_ary_push(types, rb_str_new2("numeric")); break;
            case SWIFT_TYPE_FLOAT:
                rb_ary_push(types, rb_str_new2("float")); break;
            case SWIFT_TYPE_BLOB:
                rb_ary_push(types, rb_str_new2("blob")); break;
            case SWIFT_TYPE_DATE:
                rb_ary_push(types, rb_str_new2("date")); break;
            case SWIFT_TYPE_TIME:
                rb_ary_push(types, rb_str_new2("time")); break;
            case SWIFT_TYPE_TIMESTAMP:
                rb_ary_push(types, rb_str_new2("timestamp")); break;
            case SWIFT_TYPE_BOOLEAN:
                rb_ary_push(types, rb_str_new2("boolean")); break;
            default:
                rb_ary_push(types, rb_str_new2("text"));

        }
    }
    return types;
}

void init_swift_db_mysql_typecast() {
    rb_require("bigdecimal");
    rb_require("stringio");
    rb_require("date");

    cStringIO   = CONST_GET(rb_mKernel, "StringIO");
    cBigDecimal = CONST_GET(rb_mKernel, "BigDecimal");
    cDateTime   = CONST_GET(rb_mKernel, "DateTime");
    fnew        = rb_intern("new");
    fto_date    = rb_intern("to_date");
    fstrftime   = rb_intern("strftime");
    dtformat    = rb_str_new2("%F %T.%N %z");

    rb_global_variable(&dtformat);
}
