# Swift MySQL adapter

MRI adapter for mysql for use in Swift ORM.

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

    Asynchronous API (see test/test_async.rb)

    #query(sql, *bind)
    #fileno
    #result

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


### Synchronous

```ruby
db = Swift::DB::Mysql.new(db: 'swift_test')

now = Time.now.utc
db.execute('drop table if exists users')
db.execute('create table users (id int auto_increment primary key, name text, age integer, created_at datetime)')
db.execute('insert into users(name, age, created_at) values(?, ?, ?)', 'test', 30, now)

db.execute('select * from users').first #=> {:id => 1, :name => 'test', :age => 30, :created_at=> #<Swift::DateTime>}
```

### Asynchronous

```ruby
rows = []
pool = 3.times.map {Swift::DB::Mysql.new(db: 'swift_test')}

3.times do |n|
  Thread.new do
    pool[n].query("select sleep(#{(3 - n) / 10.0}), #{n + 1} as query_id") {|row| rows << row[:query_id]}
  end
end

Thread.list.reject {|thread| Thread.current == thread}.each(&:join)
rows #=> [3, 2, 1]
```

## License

MIT
