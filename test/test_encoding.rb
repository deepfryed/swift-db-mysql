require 'helper'

describe 'utf-8 encoding' do
  before do
    assert db.execute('drop table if exists users')
    assert db.execute('create table users (name text)')
    assert db.execute('alter table users default character set utf8')
    assert db.execute('alter table users change name name text charset utf8')

    @text = ["King of \u2665s", "\xA1\xB8".force_encoding('euc-jp')]
  end

  it 'should store and retrieve utf8 characters with Statement#execute' do
    @text.each do |name|
      db.prepare('insert into users (name) values(?)').execute(name)
      value = db.prepare('select * from users limit 1').execute.first[:name]
      assert_equal Encoding::UTF_8, value.encoding
      assert_equal name.encode('utf-8'), value
      db.execute('delete from users')
    end
  end

  it 'should store and retrieve utf8 characters Adapter#execute' do
    @text.each do |name|
      db.execute('insert into users (name) values(?)', name)
      value = db.execute('select * from users limit 1').first[:name]
      assert_equal Encoding::UTF_8, value.encoding
      assert_equal name.encode('utf-8'), value
      db.execute('delete from users')
    end
  end
end
