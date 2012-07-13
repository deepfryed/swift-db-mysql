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

## License

MIT
