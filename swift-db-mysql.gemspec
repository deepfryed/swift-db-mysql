# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "swift-db-mysql"
  s.version = "0.1.0"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Bharanee Rathna"]
  s.date = "2012-07-14"
  s.description = "Swift adapter for MySQL database"
  s.email = ["deepfryed@gmail.com"]
  s.extensions = ["ext/swift/db/mysql/extconf.rb"]
  s.files = ["ext/swift/db/mysql/datetime.c", "ext/swift/db/mysql/main.c", "ext/swift/db/mysql/typecast.c", "ext/swift/db/mysql/result.c", "ext/swift/db/mysql/common.c", "ext/swift/db/mysql/statement.c", "ext/swift/db/mysql/adapter.c", "ext/swift/db/mysql/typecast.h", "ext/swift/db/mysql/datetime.h", "ext/swift/db/mysql/adapter.h", "ext/swift/db/mysql/result.h", "ext/swift/db/mysql/common.h", "ext/swift/db/mysql/statement.h", "ext/swift/db/mysql/extconf.rb", "test/helper.rb", "test/test_adapter.rb", "lib/swift/db/mysql.rb", "lib/swift-db-mysql.rb", "README.md", "CHANGELOG"]
  s.homepage = "http://github.com/deepfryed/swift-db-mysql"
  s.require_paths = ["lib", "ext"]
  s.rubygems_version = "1.8.24"
  s.summary = "Swift mysql adapter"

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_development_dependency(%q<rake>, [">= 0"])
    else
      s.add_dependency(%q<rake>, [">= 0"])
    end
  else
    s.add_dependency(%q<rake>, [">= 0"])
  end
end
