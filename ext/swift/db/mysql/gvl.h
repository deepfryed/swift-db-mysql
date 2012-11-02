// vim:ts=4:sts=4:sw=4:expandtab

// (c) Bharanee Rathna 2012

#pragma once

#include "ruby/ruby.h"
#include "ruby/intern.h"
#include "ruby/version.h"

#if RUBY_API_VERSION_MAJOR >= 2
#include "ruby/thread.h"
#define GVL_NOLOCK rb_thread_call_without_gvl
#define GVL_NOLOCK_RETURN_TYPE void*
#else
#define GVL_NOLOCK rb_thread_blocking_region
#define GVL_NOLOCK_RETURN_TYPE VALUE
#endif
