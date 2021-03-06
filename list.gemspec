# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)

Gem::Specification.new do |spec|
  spec.name          = "list"
  spec.version       = "0.2.0"
  spec.author        = "ksss"
  spec.email         = "co000ri@gmail.com"
  spec.summary       = %q{List in Ruby}
  spec.description   = %q{List is a class of wrapped singly-linked list}
  spec.homepage      = ""
  spec.license       = "MIT"

  spec.files         = `git ls-files`.split($/)
  spec.executables   = spec.files.grep(%r{^bin/}) { |f| File.basename(f) }
  spec.test_files    = spec.files.grep(%r{^(test|spec|features)/})
  spec.require_paths = ["lib"]
  spec.extensions    = ["ext/list/extconf.rb"]

  spec.required_ruby_version = ">= 2.0.0"
  spec.add_development_dependency "bundler"
  spec.add_development_dependency "rake"
  spec.add_development_dependency "rspec"
  spec.add_development_dependency "rake-compiler"
end
