require 'helper'

describe 'async operations' do
  it 'can query async and call block with result when ready' do
    rows    = []
    threads = []
    pool    = 3.times.map {Swift::DB::Mysql.new(db: 'swift_test')}

    3.times do |n|
      threads << Thread.new do
        pool[n].query("select sleep(#{(3 - n) / 10.0}), #{n + 1} as query_id") {|row| rows << row[:query_id]}
      end
    end

    threads.each(&:join)
    assert_equal [3, 2, 1], rows
  end

  it 'returns and allows IO poll on connection file descriptor' do

    rows    = []
    threads = []
    pool    = 3.times.map {Swift::DB::Mysql.new(db: 'swift_test')}

    3.times do |n|
      threads << Thread.new do
        pool[n].query("select sleep(#{(3 - n) / 10.0}), #{n + 1} as query_id")
        IO.select([IO.for_fd(pool[n].fileno)], [], [])
        rows << pool[n].result.first[:query_id]
      end
    end

    threads.each(&:join)
    assert_equal [3, 2, 1], rows
  end
end


