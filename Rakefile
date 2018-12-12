require 'date'
require 'pathname'
require 'rake'
require 'rake/clean'
require 'rake/testtask'

$rootdir = Pathname.new(__FILE__).dirname
$gemspec = Gem::Specification.new do |s|
  s.name              = 'swift-db-mysql'
  s.version           = '0.3.4'
  s.date              = Date.today
  s.authors           = ['Bharanee Rathna']
  s.email             = ['deepfryed@gmail.com']
  s.summary           = 'Swift mysql adapter'
  s.description       = 'Swift adapter for MySQL database'
  s.homepage          = 'http://github.com/deepfryed/swift-db-mysql'
  s.files             = Dir['ext/**/*.{c,h}'] + Dir['{ext,test,lib}/**/*.rb'] + %w(README.md CHANGELOG)
  s.extensions        = %w(ext/swift/db/mysql/extconf.rb)
  s.require_paths     = %w(lib ext)
  s.licenses          = %w(MIT)

  s.add_development_dependency('rake', '~> 0')
end

desc 'Generate gemspec'
task :gemspec do
  $gemspec.date = Date.today
  File.open('%s.gemspec' % $gemspec.name, 'w') {|fh| fh.write($gemspec.to_ruby)}
end

desc 'compile extension'
task :compile do
  Dir.chdir('ext/swift/db/mysql') do
    system('ruby extconf.rb && make -j2') or raise 'unable to compile mysql'
  end
end

Rake::TestTask.new(:test) do |test|
  test.libs   << 'ext' << 'lib' << 'test'
  test.pattern = 'test/**/test_*.rb'
  test.verbose = true
end

task default: :test
task :test => [:compile]

desc 'tag release and build gem'
task :release => [:test, :gemspec] do
  system("git tag -m 'version #{$gemspec.version}' v#{$gemspec.version}") or raise "failed to tag release"
  system("gem build #{$gemspec.name}.gemspec")
end
