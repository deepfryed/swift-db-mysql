#include "datetime.h"
#include <ctype.h>

extern VALUE dtformat;

VALUE cSwiftDateTime, day_seconds;
ID fcivil, fparse, fstrptime;

// NOTE: only parses '%F %T.%N %z' format and falls back to the built-in DateTime#parse
//       and is almost 2x faster than doing:
//
//       rb_funcall(klass, fstrptime, 2, rb_str_new(data, size), dtformat);
//
VALUE datetime_parse(VALUE klass, const char *data, size_t size) {
  struct tm tm;
  double seconds;
  const char *ptr;
  char tzsign = 0, fraction[32];
  int  tzhour = 0, tzmin = 0, lastmatch = -1, offset = 0, idx;

  memset(&tm, 0, sizeof(struct tm));
  sscanf(data, "%04d-%02d-%02d %02d:%02d:%02d%n",
      &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &lastmatch);

  // fallback to default datetime parser, this is more expensive.
  if (tm.tm_mday == 0)
    return Qnil;

  seconds = tm.tm_sec;

  // parse millisecs if any -- tad faster than using %lf in sscanf above.
  if (lastmatch > 0 && lastmatch < (int)size && *(data + lastmatch) == '.') {
      idx = 0;
      ptr = data + ++lastmatch;
      while (*ptr && isdigit(*ptr) && idx < 31) {
        lastmatch++;
        fraction[idx++] = *ptr++;
      }

      fraction[idx] = 0;
      seconds += (double)atoll(fraction) / pow(10, idx);
  }

  // parse timezone offsets if any - matches +HH:MM +HH MM +HHMM
  if (lastmatch > 0 && lastmatch < (int)size) {
    const char *ptr = data + lastmatch;
    while(*ptr && *ptr != '+' && *ptr != '-') ptr++;
    tzsign = *ptr++;
    if (*ptr && isdigit(*ptr)) {
      tzhour = *ptr++ - '0';
      if (*ptr && isdigit(*ptr)) tzhour = tzhour * 10 + *ptr++ - '0';
      while(*ptr && !isdigit(*ptr)) ptr++;
      if (*ptr) {
        tzmin = *ptr++ - '0';
        if (*ptr && isdigit(*ptr)) tzmin = tzmin * 10 + *ptr++ - '0';
      }
    }
  }

  if (tzsign) {
    offset = tzsign == '+'
      ? (time_t)tzhour *  3600 + (time_t)tzmin *  60
      : (time_t)tzhour * -3600 + (time_t)tzmin * -60;
  }

  return rb_funcall(klass, fcivil, 7,
    INT2FIX(tm.tm_year), INT2FIX(tm.tm_mon), INT2FIX(tm.tm_mday),
    INT2FIX(tm.tm_hour), INT2FIX(tm.tm_min), DBL2NUM(seconds),
    offset == 0 ? INT2FIX(0) : rb_Rational(INT2FIX(offset), day_seconds)
  );
}

VALUE rb_datetime_parse(VALUE self, VALUE string) {
  VALUE datetime;
  const char *data = CSTRING(string);
  size_t size = TYPE(string) == T_STRING ? (size_t)RSTRING_LEN(string) : strlen(data);

  if (NIL_P(string))
    return Qnil;

  datetime = datetime_parse(self, data, size);
  return NIL_P(datetime) ? rb_call_super(1, &string) : datetime;
}

void init_swift_datetime() {
  VALUE mSwift, cDateTime;

  rb_require("date");
  mSwift          = rb_define_module("Swift");
  cDateTime       = CONST_GET(rb_mKernel, "DateTime");
  cSwiftDateTime  = rb_define_class_under(mSwift, "DateTime", cDateTime);
  fcivil          = rb_intern("civil");
  fparse          = rb_intern("parse");
  fstrptime       = rb_intern("strptime");
  day_seconds     = INT2FIX(86400);

  rb_global_variable(&day_seconds);
  rb_define_singleton_method(cSwiftDateTime, "parse", RUBY_METHOD_FUNC(rb_datetime_parse), 1);
}
