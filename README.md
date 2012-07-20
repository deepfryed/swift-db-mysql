# Swift MySQL adapter

MRI adapter for MySQL

## Features

* Lightweight & fast
* Result typecasting
* Prepared statements, yeah finally!
* Asynchronous support
* Nested Transactions

## API

```
  Swift::DB::Mysql
    .new(options)
    #execute(sql, *bind)
    #prepare(sql)
    #begin(savepoint = nil)
    #commit(savepoint = nil)
    #rollback(savepoint = nil)
    #transaction(savepoint = nil, &block)
    #close
    #closed?
    #escape(text)
    #query(sql, *bind)
    #fileno
    #result
    #write(table, fields = nil, io_or_string)

  Swift::DB::MySql::Statement
    .new(Swift::DB::Mysql, sql)
    #execute(*bind)
    #release

  Swift::DB::Mysql::Result
    #selected_rows
    #affected_rows
    #fields
    #types
    #each
    #insert_id
```

## Example

```ruby
require 'swift/db/mysql'

db = Swift::DB::Mysql.new(db: 'swift_test')

db.execute('drop table if exists users')
db.execute('create table users (id serial, name text, age integer, created_at datetime)')
db.execute('insert into users(name, age, created_at) values(?, ?, ?)', 'test', 30, Time.now.utc)

row = db.execute('select * from users').first
p row #=> {:id => 1, :name => 'test', :age => 30, :created_at=> #<Swift::DateTime>}
```

### Asynchronous

Hint: You can use `Adapter#fileno` and `EventMachine.watch` if you need to use this with EventMachine.

```ruby
require 'swift/db/mysql'

rows = []
pool = 3.times.map {Swift::DB::Mysql.new(db: 'swift_test')}

3.times do |n|
  Thread.new do
    pool[n].query("select sleep(#{(3 - n) / 10.0}), #{n + 1} as query_id") do |row|
      rows << row[:query_id]
    end
  end
end

Thread.list.reject {|thread| Thread.current == thread}.each(&:join)
p rows #=> [3, 2, 1]
```

### Data I/O

The adapter supports data write via LOAD DATA LOCAL INFILE command.

```ruby
require 'swift/db/mysql'

db = Swift::DB::Mysql.new(db: 'swift_test')
db.execute('drop table if exists users')
db.execute('create table users (id int auto_increment primary key, name text)')

db.write('users', %w{name}, "foo\nbar\nbaz\n")
db.write('users', %w{name}, StringIO.new("foo\nbar\nbaz\n"))
db.write('users', %w{name}, File.open("users.dat"))
```

## Performance

Don't read too much into it. Each library has its advantages and disadvantages.

```
# insert 1000 rows and read them back 100 times

$ ruby check.rb
                      user     system      total        real
mysql2 insert     0.230000   0.040000   0.270000 (  0.520786)
mysql2 select     1.190000   0.260000   1.450000 (  1.465079)
swift insert      0.100000   0.060000   0.160000 (  0.496894)
swift select      0.290000   0.000000   0.290000 (  0.294259)
```

## License

MIT
