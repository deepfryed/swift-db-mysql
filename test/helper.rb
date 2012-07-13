require 'minitest/autorun'
require 'swift/db/mysql'

class MiniTest::Spec
  def db
    @db ||= Swift::DB::Mysql.new(db: 'swift_test')
  end
end
