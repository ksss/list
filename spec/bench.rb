#! /usr/bin/env ruby

lib = File.expand_path('../../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)

require 'list'
require 'benchmark'

Benchmark.bm(32) do |x|
  [1000,10000,100000].each do |n|
    x.report("#{n}times") do
      n.times do
      end
    end
  end

  [Array, List].each do |obj|
    [1000,10000,100000].each do |n|
      x.report("#{obj}#new #{n}times") do
        n.times do
          obj.new
        end
      end
    end
  end

  [[:push, 1], [:unshift, 1], [:pop], [:shift], [:insert, 0, 1], [:delete_at, 0]].each do |args|
    m = args.shift
    [(0..100000).to_a, (0..100000).to_list].each do |obj|
      [1000,10000,100000].each do |n|
        o = obj.dup
        x.report("#{o.class}##{m} #{n}times") do
          n.times do
            o.send(m, *args)
          end
        end
      end
    end
  end
end
