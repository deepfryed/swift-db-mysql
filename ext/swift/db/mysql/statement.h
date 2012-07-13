// vim:ts=4:sts=4:sw=4:expandtab

// (c) Bharanee Rathna 2012

#pragma once

#include "common.h"

typedef struct Statement {
    MYSQL_STMT *statement;
    VALUE adapter;
} Statement;

void init_swift_db_mysql_statement();
