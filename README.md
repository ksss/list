# List

List in Ruby.

[![Build Status](https://travis-ci.org/ksss/list.png?branch=master)](https://travis-ci.org/ksss/list)

## Usage

All interface is same with Array class.

```ruby
list = List.new
list.push 1,2,3
list.pop
list[0,1]
list.each do |i|
end
```

    +---------+  +->+---------+  +->+---------+
    |  value  |  |  |  value  |  |  |  value  |
    |  next   |--+  |  next   |--+  |  next   |
    +---------+     +---------+     +---------+

But, List is not Array.

```ruby
list = List[1,2,3]
list.ring.each do |i|
  puts i # print infinitely 1,2,3,1,2,3,1,2...
end
```

    +->+---------+  +->+---------+  +->+---------+
    |  |  value  |  |  |  value  |  |  |  value  |
    |  |  next   |--+  |  next   |--+  |  next   |--+
    |  +---------+     +---------+     +---------+  |
    +-----------------------------------------------+

## Installation

Add this line to your application's Gemfile:

    gem 'list'

And then execute:

    $ bundle

Or install it yourself as:

    $ gem install list

## Contributing

1. Fork it ( http://github.com/ksss/list/fork )
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request
