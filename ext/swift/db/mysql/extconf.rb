#!/usr/bin/env ruby

require 'mkmf'

$CFLAGS = '-std=c99 -Os'

inc_paths = %w(
  /usr/include
  /usr/include/mysql
  /usr/local/include
  /usr/local/include/mysql
  /opt/local/include
  /opt/local/include/mysql
  /opt/local/include/mysql5
  /sw/include
)

lib_paths = %w(
  /usr/lib
  /usr/local/lib
  /opt/local/lib
  /opt/local/lib/mysql5/mysql
  /sw/lib
  /usr/lib64/mysql
  /usr/lib32/mysql
)

uuid_inc,  uuid_lib  = dir_config('uuid',  '/usr/include/uuid', '/usr/lib')
mysql_inc, mysql_lib = dir_config('mysql')

find_header 'uuid.h',  *inc_paths.dup.unshift(uuid_inc).compact
find_header 'mysql.h', *inc_paths.dup.unshift(mysql_inc).compact

find_library 'uuid',        'main', *lib_paths.dup.unshift(uuid_lib).compact
find_library 'mysqlclient', 'main', *lib_paths.dup.unshift(mysql_lib).compact

create_makefile('swift_db_mysql_ext')
