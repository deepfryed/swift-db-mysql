# Swift MySQL adapter

MRI adapter for MySQL

## Features

* Lightweight & fast
* Result typecasting
* Prepared statements, yeah finally!
* Asynchronous support

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
    #write(table = nil, fields = nil, io_or_string)

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

### Asynchronous API

```
  Swift::DB::Mysql
    #query(sql, *bind)
    #fileno
    #result
```

### Data I/O API

```
  Swift::DB::Mysql
    #write(table = nil, fields = nil, io_or_string)
```

## Example


### Synchronous

```ruby
require 'swift/db/mysql'

db = Swift::DB::Mysql.new(db: 'swift_test')

db.execute('drop table if exists users')
db.execute('create table users (id int auto_increment primary key, name text, age integer, created_at datetime)')
db.execute('insert into users(name, age, created_at) values(?, ?, ?)', 'test', 30, Time.now.utc)

p db.execute('select * from users').first #=> {:id => 1, :name => 'test', :age => 30, :created_at=> #<Swift::DateTime>}
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

## License

MIT
