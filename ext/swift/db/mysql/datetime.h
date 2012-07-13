#pragma once

#include "common.h"
#include <math.h>

DLL_PRIVATE extern VALUE cSwiftDateTime;
DLL_PRIVATE void init_swift_datetime();
DLL_PRIVATE VALUE datetime_parse(VALUE klass, const char *data, size_t size);
