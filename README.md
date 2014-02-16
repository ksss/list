# List

List in Ruby.

[![Build Status](https://travis-ci.org/ksss/list.png?branch=master)](https://travis-ci.org/ksss/list)

**List** is a class of wrapped singly-linked list.

It's can use same as **Array**. But processing speed is different.

## Usage

All interface is same with Array class.

```
> Array.methods - List.methods
=> []
> Array.new.methods - List.new.methods
=> []
```

List can use instead of Array.

```ruby
require 'list'

list = List.new # or List[]
list.push 1,2,3
list.pop #=> 3
list[0,2] #=> List[1,2]
list[0,1] = 5
list.each do |i|
  puts i #=> 5,2
end
puts List[1,2,3].map{|i| i * i}.inject(:+) #=> 14
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

## Feature

- List have all same method with Array.
- **insert** and **delete** is more faster than Array.
- Can use MRI at version 1.9.3, 2.0.0 and 2.1.0.

## Other API

`List#ring`: experimental method for expressing the ring buffer.

`List#ring!`: change self to ring buffer.

`List#ring?`: check self to ring buffer.

`Enumeratable#to_list`: all class of included Enumeratable, can convert to List instance

`List#to_list`: is for duck typing this.

## Installation

Add this line to your application's Gemfile:

    gem 'list'

And then execute:

    $ bundle

Or install it yourself as:

    $ gem install list

## Testing

This library is tested using [Travis](https://travis-ci.org/ksss/list).

- MRI 1.9.3
- MRI 2.0.0
- MRI 2.1.0

## Contributing

1. Fork it ( http://github.com/ksss/list/fork )
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request

## Other

[RDoc for Array](http://ruby-doc.org/core-2.1.0/Array.html)

[benchmark script result](https://gist.github.com/ksss/8760144)

[wikipedia](http://en.wikipedia.org/wiki/Linked_list)
