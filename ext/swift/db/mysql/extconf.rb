#!/usr/bin/env ruby

require 'mkmf'

$CFLAGS = '-std=c99 -Os'

inc_paths = %w(
  /usr/include
  /usr/include/mysql
  /usr/local/include
  /usr/local/include/mysql
  /usr/local/mysql/include
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

if File.exists?(%x{which mysql_config}.strip)
  %x{mysql_config --include}.strip.gsub(/-I/, '').split(/\s+/).each do |dir|
    inc_paths.unshift dir
  end

  %x{mysql_config --libs}.strip.scan(/-L([^ ]+)/).flatten.each do |dir|
    lib_paths.unshift dir
  end
end

uuid_inc,  uuid_lib  = dir_config('uuid',  '/usr/include/uuid', '/usr/lib')
mysql_inc, mysql_lib = dir_config('mysql')

find_header 'uuid.h',  *inc_paths.dup.unshift(uuid_inc).compact
find_header 'mysql.h', *inc_paths.dup.unshift(mysql_inc).compact

find_library 'uuid',        'main', *lib_paths.dup.unshift(uuid_lib).compact
find_library 'mysqlclient', 'main', *lib_paths.dup.unshift(mysql_lib).compact

have_const(%w(UUID_VARIANT_NCS), 'uuid.h')
create_makefile('swift_db_mysql_ext')
