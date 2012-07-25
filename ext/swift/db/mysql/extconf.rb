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
)

(inc_paths << ENV['SMY_INCLUDE_DIRS']).compact!
(lib_paths << ENV['SMY_LIBRARY_DIRS']).compact!

find_header('mysql.h',     *inc_paths) or raise 'unable to locate mysql headers set SMY_INCLUDE_DIRS'
find_header('uuid/uuid.h', *inc_paths) or raise 'unable to locate uuid headers set SMY_INCLUDE_DIRS'

find_library('mysqlclient',  'main', *lib_paths) or raise 'unable to locate mysql lib set SMY_LIBRARY_DIRS'
find_library('uuid',         'main', *lib_paths) or raise 'unable to locate uuid lib set SMY_LIBRARY_DIRS'

create_makefile('swift_db_mysql_ext')
