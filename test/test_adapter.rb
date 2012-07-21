require 'helper'

describe 'mysql adapter' do
  it 'should initialize' do
    assert db
  end

  it 'should execute sql' do
    assert db.execute("select * from information_schema.tables limit 1")
  end

  it 'should expect the correct number of bind args' do
    assert_raises(Swift::ArgumentError) { db.execute("select * from information_schema.tables where table_name = ?", 1, 2) }
  end

  it 'should return result on #execute' do
    now = Time.now.utc
    assert db.execute('drop table if exists users')
    assert db.execute('create table users (id int auto_increment primary key, name text, age integer, created_at datetime)')
    assert db.execute('insert into users(name, age, created_at) values(?, ?, ?)', 'test', nil, now)

    result = db.execute('select * from users')

    assert_equal 1, result.selected_rows
    assert_equal 0, result.affected_rows
    assert_equal %w(id name age created_at).map(&:to_sym), result.fields
    assert_equal %w(integer text integer timestamp), result.types

    row = result.first
    assert_equal 1,      row[:id]
    assert_equal 'test', row[:name]
    assert_equal nil,    row[:age]
    assert_equal now.to_i, row[:created_at].to_time.to_i

    result = db.execute('delete from users where id = 0')
    assert_equal 0, result.selected_rows
    assert_equal 0, result.affected_rows

    assert_equal 1, db.execute('select count(*) as count from users').first[:count]

    result = db.execute('delete from users')
    assert_equal 0, result.selected_rows
    assert_equal 1, result.affected_rows
  end

  it 'should close handle' do
    assert db.ping
    assert !db.closed?
    assert db.close
    assert db.closed?
    assert !db.ping

    assert_raises(Swift::ConnectionError) { db.execute("select * from users") }
  end

  it 'should prepare & release statement' do
    assert db.execute('drop table if exists users')
    assert db.execute("create table users(id int auto_increment primary key, name text, created_at datetime)")
    assert db.execute("insert into users (name, created_at) values (?, ?)", "test", Time.now)
    assert s = db.prepare("select * from users where id > ? and created_at <= ?")

    assert_equal 0, s.execute(1, Time.now).selected_rows
    assert_equal 1, s.execute(0, Time.now).selected_rows

    assert s.release
    assert_raises(Swift::RuntimeError) { s.execute(1, Time.now) }
  end

  it 'should escape whatever' do
    assert_equal "foo\\'bar", db.escape("foo'bar")
  end

  it 'should support #write' do
    assert db.execute('drop table if exists users')
    assert db.execute("create table users(id int auto_increment primary key, name text)")

    assert_equal 3, db.write('users', %w(name), "foo\nbar\nbaz\n").affected_rows
    assert_equal 3, db.execute('select count(*) as count from users').first[:count]

    assert_equal 3, db.write('users', StringIO.new("7\tfoo\n8\tbar\n9\tbaz\n")).affected_rows
    assert_equal 6, db.execute('select count(*) as count from users').first[:count]
  end
end
