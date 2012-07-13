#!/usr/bin/env ruby

$:.unshift File.dirname(__FILE__) + '/../ext'
$:.unshift File.dirname(__FILE__) + '/../lib'

require 'bundler/setup'
require 'mysql2'
require 'swift-db-mysql'
require 'benchmark'

dbs = {
  mysql2:  Mysql2::Client.new(database: 'swift_test'),
  swift:   Swift::DB::Mysql.new(db: 'swift_test')
}

sql = {
  drop:   'drop table if exists users',
  create: 'create table users(id integer auto_increment primary key, name text, created_at datetime)',
  insert: 'insert into users(name, created_at) values (?, ?)',
  update: 'update users set name = ?, created_at = ? where id = ?',
  select: 'select * from users where id > ?'
}

rows = 1000
iter = 100

class Mysql2::Client
  def execute sql, *args
    sql = sql.dup
    # simple search & replace interpolation.
    loop do
      break unless sql.sub!(/\?/) { (v = args.shift) ? "'%s'" % escape(v.to_s) : 'NULL' }
    end
    query(sql)
  end
end

Benchmark.bm(15) do |bm|
  dbs.each do |name, db|
    db.execute(sql[:drop])
    db.execute(sql[:create])

    bm.report("#{name} insert") do
      rows.times do |n|
        db.execute(sql[:insert], "name #{n}", Time.now.to_s)
      end
    end

    bm.report("#{name} select") do
      iter.times do
        db.execute(sql[:select], 0).entries
      end
    end
  end
end
