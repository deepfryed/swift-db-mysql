#pragma once

#include <ruby.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

extern VALUE cSwiftDateTime;
void init_swift_datetime();
VALUE datetime_parse(VALUE klass, const char *data, size_t size);
