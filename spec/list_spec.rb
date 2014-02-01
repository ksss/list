require 'spec_helper'

describe List do
  before :each do
    GC.start
  end

  after :each do
    GC.start
  end

  it "class" do
    expect(List).to be_include(Enumerable)
  end

  it "create" do
    expect(List[]).to eq(List.new)
    expect(List[1,2,3]).to eq(List.new [1,2,3])
    expect(L[1,2,3]).to eq(L.new [1,2,3])
  end

  it "try_convert" do
    expect(List.try_convert List[1,2,3]).to eq(List[1,2,3])
    expect(List.try_convert [1,2,3]).to eq(List[1,2,3])
    expect(List.try_convert "[1,2,3]").to eq(nil)
  end

  it "to_list" do
    expect([].to_list).to eq(List.new)
    expect([1,[2],3].to_list).to eq(List[1,[2],3])
    expect((0..5).to_list).to eq(List[0,1,2,3,4,5])
    expect(List[1,2,3].to_list).to eq(List[1,2,3])
    expect(L[1,2,3].to_list).to eq(L[1,2,3])
  end

  it "initialize" do
    expect(List.new).to be_a_kind_of(List)
    expect(List.new([])).to be_a_kind_of(List)
    expect(List.new([1,2,3])).to be_a_kind_of(List)
    expect(List.new([1,2,3])).to eq([1,2,3].to_list)
    expect(List.new(3)).to eq([nil,nil,nil].to_list)
    expect(List.new(3, 0)).to eq([0,0,0].to_list)
    expect(List.new(3){|i| "foo"}).to eq(["foo", "foo", "foo"].to_list)
    expect{List.new(3,0,0)}.to raise_error(ArgumentError)
    expect{List.new("a")}.to raise_error(TypeError)
    expect{List.new("a",0)}.to raise_error(TypeError)
  end

  it "dup and replace" do
    list = List.new
    expect(list.dup).to eq(list)
    list = List[1,2,3]
    expect(list.dup).to eq(list)
  end

  it "inspect" do
    list = List.new
    expect(list.inspect).to eq("#<List: []>")
    list.push 1,[2],{:a=>3}
    expect(list.inspect).to eq("#<List: [1, [2], {:a=>3}]>")
    expect(L[1,2,3].inspect).to eq("#<L: [1, 2, 3]>")
  end

  it "to_a" do
    list = List.new
    expect(list.to_a).to eq([])
    a = (0..10).to_a
    list = List.new(a)
    expect(list.to_a).to eq(a)
  end

  it "freeze and frozen?" do
    list = List.new
    list.freeze
    expect(list).to eq(List[])
    expect(list.frozen?).to eq(true)
    expect{ list.push 1 }.to raise_error
  end

  it "==" do
    list = List.new
    expect(list).to eq(List.new)
    expect(list).to eq(list)
    list.push(1)
    expect(list).to_not eq(List.new)
    expect(list).to eq(List[1])
    expect(list).to_not eq([1])
    expect(L[1]).to eq(L[1])
    expect(L[1]).to eq(List[1])
  end

  it "eql?" do
    list = List.new
    expect(list.eql?(list)).to be true
    expect(list.eql?(List.new)).to be false
    expect(list.eql?(L.new)).to be false
  end

  it "hash" do
    expect(List.new.hash).to eq(List.new.hash)
    list1 = List[1,2,3]
    list2 = List[1,2,3]
    expect(list1.hash).to eq(list2.hash)
    hash = {}
    hash[list1] = 1
    expect(hash[list1]).to eq(1)
  end

  it "[]" do
    list = List.new
    expect(list[0]).to eq(nil)
    list.push 1,2,3
    expect(list[2]).to eq(3)
    expect(list[-1]).to eq(3)
    expect(list[10]).to eq(nil)
    expect(list[1,1]).to eq(List[2])
    expect(list[1,2]).to eq(List[2,3])
    expect(list[1,10]).to eq(List[2,3])

    # special cases
    expect(list[3]).to eq(nil)
    expect(list[10,0]).to eq(nil)
    expect(list[3,0]).to eq(List[])
    expect(list[3..10]).to eq(List[])
  end

  it "[]=" do
    list = List.new
    list[0] = 1
    expect(list).to eq(List[1])
    list[1] = 2
    expect(list).to eq(List[1,2])
    list[4] = 4
    expect(list).to eq(List[1,2,nil,nil,4])
    list[0,3] = List['a','b','c']
    expect(list).to eq(List['a','b','c',nil,4])
    list[0,3] = List['a','b','c']
    expect(list).to eq(List['a','b','c',nil,4])
    list[1..2] = List[1,2]
    expect(list).to eq(List['a',1,2,nil,4])
    list[0,2] = "?"
    expect(list).to eq(List["?",2,nil,4])
    list[0..2] = "A"
    expect(list).to eq(List["A",4])
    list[-1] = "Z"
    expect(list).to eq(List["A","Z"])
    list[1..-1] = nil
    expect(list).to eq(List["A",nil])
    list[1..-1] = List[]
    expect(list).to eq(List["A"])
    list[0,0] = List[1,2]
    expect(list).to eq(List[1,2,"A"])
    list[3,0] = "B"
    expect(list).to eq(List[1,2,"A","B"])
    list[6,0] = "C"
    expect(list).to eq(List[1,2,"A","B",nil,nil,"C"])
    expect{list["10"]}.to raise_error(TypeError)

    a = List[1,2,3]
    a[1,0] = a
    expect(a).to eq(List[1,1,2,3,2,3]);

    a = List[1,2,3]
    a[-1,0] = a
    expect(a).to eq(List[1,2,1,2,3,3]);
  end

  it "at" do
    list = List.new
    expect(list.at(0)).to eq(nil)
    list.push 1,2,3
    expect(list.at(0)).to eq(1)
    expect(list.at(2)).to eq(3)
    expect(list.at(3)).to eq(nil)
    expect{list.at("10")}.to raise_error(TypeError)
  end

  it "fetch" do
    list = List[11,22,33,44]
    expect(list.fetch(1)).to eq(22)
    expect(list.fetch(-1)).to eq(44)
    expect(list.fetch(4, 'cat')).to eq('cat')
    expect(list.fetch(100){|i| "#{i} is out of bounds" }).to eq('100 is out of bounds')
    expect{list.fetch(100)}.to raise_error(IndexError)
    expect{list.fetch("100")}.to raise_error(TypeError)
  end

  it "first" do
    list = List.new
    expect(list.first).to eq(nil)
    list.push 1,2,3
    expect(list.first).to eq(1)
    expect(list.first).to eq(1)
    expect(list.first(2)).to eq(List[1,2])
    expect(list.first(10)).to eq(List[1,2,3])
    expect{list.first("1")}.to raise_error(TypeError)
    expect{list.first(-1)}.to raise_error(ArgumentError)
  end

  it "last" do
    list = List.new
    expect(list.last).to eq(nil)
    list.push 1,2,3
    expect(list.last).to eq(3)
    expect(list.last).to eq(3)
    expect(list.last(2)).to eq(List[2,3])
    expect(list.last(10)).to eq(List[1,2,3])
    expect{list.last("1")}.to raise_error(TypeError)
    expect{list.last(-1)}.to raise_error(ArgumentError)
  end

  it "concat" do
    list = List.new
    list2 = List[1,2,3]
    expect(list.concat(list2)).to eq(List[1,2,3])
    expect(list.concat([4,5,6])).to eq(List[1,2,3,4,5,6])
    expect(list.concat(L[7,8,9])).to eq(List[1,2,3,4,5,6,7,8,9])
  end

  it "push" do
    list = List.new
    expect(list.push).to eq(list)
    10.times { |i|
      list << i
    }
    expect(list).to eq((0...10).to_list)
    list.push(*(10...20).to_a)
    expect(list).to eq((0...20).to_list)
  end

  it "pop" do
    list = List.new
    expect(list.pop).to eq(nil)
    list.push 1,2,3
    expect(list.pop(2).to_a).to eq([2,3])
    expect(list.pop).to eq(1)
    expect{list.pop("1")}.to raise_error(TypeError) 
  end

  it "shift" do
    list = List.new
    expect(list.shift).to eq(nil)
    list.push 1,2,3
    expect(list.shift(2).to_a).to eq([1,2])
    expect(list.shift).to eq(3)
    expect{list.shift("1")}.to raise_error(TypeError) 
  end

  it "unshift" do
    list = List.new
    expect(list.unshift).to eq(list)
    expect(list.unshift(3).to_a).to eq([3])
    expect(list.unshift(2,1).to_a).to eq([1,2,3])
  end

  it "insert" do
    list = List['a','b','c','d']
    expect(list.insert(2, 99)).to eq(List["a","b",99,"c","d"]);
    expect{list.insert("a", 1)}.to raise_error(TypeError)
  end

  it "each" do
    list = List.new
    expect(list.each).to be_a_kind_of(Enumerator)
    expect(list.each{}).to eq(list)

    result = []
    list.each do |i|
      result << i
    end
    expect(result).to eq([])

    list.push 1,2,3
    list.each do |i|
      result << i
    end
    expect(result).to eq([1,2,3])
  end

  it "each_index" do
    list = List.new
    expect(list.each_index).to be_a_kind_of(Enumerator)
    expect(list.each_index{}).to eq(list)

    result = []
    list.each_index do |i|
      result << i
    end
    expect(result).to eq([])

    list.push 1,2,3
    list.each_index do |i|
      result << i
    end
    expect(result).to eq([0,1,2])
  end

  it "reverse_each" do
    list = List[1,2,3]
    result = []
    list.reverse_each do |i|
      result << i
    end
    expect(result).to eq([3,2,1])
  end

  it "length" do
    list = List.new
    expect(list.length).to eq(0)
    list.push 1,2,3
    expect(list.length).to eq(3)
    expect(list.size).to eq(3)
  end

  it "empty?" do
    list = List.new
    expect(list.empty?).to eq(true)
    list.push 1,2,3
    expect(list.empty?).to eq(false)
  end

  it "find_index" do
    list = List.new
    expect(list.find_index(1)).to eq(nil)
    list.push 1,2,3
    expect(list.find_index(1)).to eq(0)
    expect(list.find_index(2)).to eq(1)
    expect(list.index(3)).to eq(2)
  end

  it "replace" do
    list = List[1,2,3,4,5]
    expect(list.replace(List[4,5,6])).to eq(List[4,5,6])
    expect(list.replace([7,8,9])).to eq(List[7,8,9])
    expect{list.replace "1"}.to raise_error(TypeError)
  end

  it "clear" do
    list = List[1,2,3]
    expect(list.clear).to eq(List[])
    expect(list.clear).to eq(List[])
  end

  it "rindex" do
    list = List[1,2,2,2,3]
    expect(list.rindex(2)).to eq(3)
    expect(list.rindex(-10)).to eq(nil)
    expect(list.rindex { |x| x == 2 }).to eq(3)
  end

  it "join" do
    expect(List.new.join).to eq("")
    expect(List[1,2,3].join).to eq("123")
    expect(List["a","b","c"].join).to eq("abc")
    expect(List["a","b","c"].join("-")).to eq("a-b-c")
    expect(List["a",List["b",List["c"]]].join("-")).to eq("a-b-c")
    expect{List["a","b","c"].join(1)}.to raise_error(TypeError)
    orig_sep = $,
    $, = "*"
    expect(List["a","b","c"].join).to eq("a*b*c")
    $, = orig_sep
  end

  it "reverse" do
    list = List.new
    expect(list.reverse).to eq(List.new)
    list.push 1,2,3
    expect(list.reverse).to eq(List[3,2,1])
    expect(list).to eq(List[1,2,3])
  end

  it "reverse!" do
    list = List.new
    expect(list.reverse!).to eq(List.new)
    list.push 1,2,3
    expect(list.reverse!).to eq(List[3,2,1])
    expect(list).to eq(List[3,2,1])
  end

  it "rotate" do
    list = List.new
    expect(list.rotate).to eq(List.new)
    list.push 1,2,3
    expect(list.rotate).to eq(List[2,3,1])
    expect(list.rotate(2)).to eq(List[3,1,2])
    expect(list.rotate(-2)).to eq(List[2,3,1])
    expect(list).to eq(List[1,2,3])
    expect{list.rotate("a")}.to raise_error(TypeError)
    expect{list.rotate(1,2)}.to raise_error(ArgumentError)
  end

  it "rotate!" do
    list = List.new
    expect(list.rotate!).to eq(List.new)
    list.push 1,2,3
    expect(list.rotate!).to eq(List[2,3,1])
    expect(list.rotate!(2)).to eq(List[1,2,3])
    expect(list.rotate!(-2)).to eq(List[2,3,1])
    expect(list).to eq(List[2,3,1])
    expect{list.rotate!("a")}.to raise_error(TypeError)
    expect{list.rotate!(1,2)}.to raise_error(ArgumentError)
  end

  it "sort" do
    list = List.new
    expect(list.sort).to eq(List.new)
    list.push *[4,1,3,5,2]
    expect(list.sort).to eq(List[1,2,3,4,5])
    expect(list.sort{|a,b| b - a}).to eq(List[5,4,3,2,1])
    expect(list).to eq(List[4,1,3,5,2])
  end

  it "sort!" do
    list = List.new
    expect(list.sort!).to eq(List.new)
    list.push *[4,1,3,5,2]
    expect(list.sort!).to eq(List[1,2,3,4,5])
    expect(list).to eq(List[1,2,3,4,5])
    expect(list.sort!{|a,b| b - a}).to eq(List[5,4,3,2,1])
    expect(list).to eq(List[5,4,3,2,1])
  end

  it "sort_by" do
    list = List.new
    expect(list.sort_by{|a| a}).to eq(List.new)
    list.push *[4,1,3,5,2]
    expect(list.sort_by{|a| a}).to eq(List[1,2,3,4,5])
    expect(list.sort_by{|a| -a}).to eq(List[5,4,3,2,1])
    expect(list).to eq(List[4,1,3,5,2])
  end

  it "sort_by!" do
    list = List.new
    expect(list.sort_by!{|a| a}).to eq(List.new)
    list.push *[4,1,3,5,2]
    expect(list.sort_by!{|a| a}).to eq(List[1,2,3,4,5])
    expect(list.sort_by!{|a| -a}).to eq(List[5,4,3,2,1])
    expect(list).to eq(List[5,4,3,2,1])
  end

  it "collect" do
    list = List.new
    expect(list.collect{|a| a}).to eq(List.new)
    list.push *[4,1,3,5,2]
    expect(list.collect{|a| a * a}).to eq(List[16,1,9,25,4])
    expect(list.map.each{|a| -a}).to eq(List[-4,-1,-3,-5,-2])
    expect(list).to eq(List[4,1,3,5,2])
  end

  it "collect!" do
    list = List.new
    expect(list.collect!{|a| a}).to eq(List.new)
    list.push *[4,1,3,5,2]
    expect(list.collect!{|a| a * a}).to eq(List[16,1,9,25,4])
    expect(list.map!.each{|a| -a}).to eq(List[-16,-1,-9,-25,-4])
  end

  it "select" do
    list = List.new
    expect(list.select{|i| i}).to eq(List.new)
    list.push *[4,1,3,5,2]
    expect(list.select{|i| i.even?}).to eq(List[4,2])
    expect(list.select.each{|i| i.odd?}).to eq(List[1,3,5])
    expect(list).to eq(List[4,1,3,5,2])
  end

  it "select!" do
    list = List.new
    expect(list.select!{|i| i}).to eq(nil)
    list.push *[4,1,3,5,2]
    expect(list.select!{|i| i.even?}).to eq(List[4,2])
    expect(list.select!.each{|i| i % 4 == 0}).to eq(List[4])
    expect(list.select!.each{|i| i % 4 == 0}).to eq(nil)
    expect(list).to eq(List[4])
  end

  it "keep_if" do
    list = List.new
    expect(list.keep_if{|i| i}).to eq(list)
    list.push *[4,1,3,5,2]
    expect(list.keep_if{|i| i.even?}).to eq(List[4,2])
    expect(list.keep_if.each{|i| i % 4 == 0}).to eq(List[4])
    expect(list.keep_if.each{|i| i % 4 == 0}).to eq(list)
    expect(list).to eq(List[4])
  end

  it "values_at" do
    expect(List.new.values_at(1)).to eq(List[nil])
    list = List[4,1,3,5,2]
    expect(list.values_at).to eq(List.new)
    expect(list.values_at 1).to eq(List[1])
    expect(list.values_at 1,3,10).to eq(List[1,5,nil])
    expect(list.values_at -1,-2).to eq(List[2,5])
    expect(list.values_at 1..5).to eq(List[1,3,5,2,nil])
    expect(list.values_at -2..-1).to eq(List[5,2])
    expect(list.values_at -1..-2).to eq(List.new)
    expect(list.values_at 0,1..5).to eq(List[4,1,3,5,2,nil])
    expect(list.values_at 10..12).to eq(List[nil,nil,nil])
    expect{list.values_at "a"}.to raise_error(TypeError)
  end

  it "delete" do
    expect(List.new.delete(nil)).to eq(nil)
    list = List[4,1,3,2,5,5]
    expect(list.delete(1)).to eq(1)
    expect(list).to eq(List[4,3,2,5,5])
    expect(list.delete(5){"not found"}).to eq(5)
    expect(list).to eq(List[4,3,2])
    expect(list.delete(4)).to eq(4)
    expect(list.delete(1){"not found"}).to eq("not found")

    list = Array.new(1024,0).to_list
    expect(list.delete(0)).to eq(0)
    expect(list).to eq(List.new)
  end

  it "delete_at" do
    list = List[1,2,3]
    expect(list.delete_at(1)).to eq(2)
    expect(list).to eq(List[1,3])
    expect(list.delete_at(-1)).to eq(3)
    expect(list).to eq(List[1])
    expect(list.delete_at(0)).to eq(1)
    expect(list).to eq(List[])
    expect(list.delete_at(0)).to eq(nil)
    expect{list.delete_at("a")}.to raise_error(TypeError)
  end

  it "delete_if" do
    list = List[1,2,3,4,5]
    expect(list.delete_if{|i| 3 < i}).to eq(List[1,2,3])
    expect(list.delete_if.each{|i| i.even?}).to eq(List[1,3])
    expect(list.delete_if{|i| 0 < i}).to eq(List[])
    expect(list.delete_if{|i| 0 < i}).to eq(List[])
    expect(list).to eq(List[])
  end

  it "reject" do
    list = List[1,2,3,4,5]
    expect(list.reject{|i| 3 < i}).to eq(List[1,2,3])
    expect(list.reject.each{|i| i.even?}).to eq(List[1,3,5])
    expect(list.reject{|i| 0 < i}).to eq(List[])
    expect(list).to eq(List[1,2,3,4,5])
  end

  it "reject!" do
    list = List[1,2,3,4,5]
    expect(list.reject!{|i| i % 10 == 0}).to eq(nil)
    expect(list.reject!{|i| 3 < i}).to eq(List[1,2,3])
    expect(list.reject!.each{|i| i.even?}).to eq(List[1,3])
    expect(list.reject!{|i| 0 < i}).to eq(List[])
    expect(list.reject!{|i| 0 < i}).to eq(nil)
    expect(list).to eq(List[])
  end

  it "zip" do
    a = List[4,5,6]
    b = List[7,8,9]
    ary = []
    expect(a.zip(b)).to eq(List[List[4,7],List[5,8],List[6,9]])
    expect(List[1,2,3].zip(a,b)).to eq(List[List[1,4,7],List[2,5,8],List[3,6,9]])
    expect(List[1,2].zip(a,b)).to eq(List[List[1,4,7],List[2,5,8]])
    expect(a.zip(List[1,2],List[8])).to eq(List[List[4,1,8],List[5,2,nil],List[6,nil,nil]])
    expect(a.zip([1,2],[8])).to eq(List[List[4,1,8],List[5,2,nil],List[6,nil,nil]])
    expect(a.zip([1,2],[8]){|i| ary << i}).to eq(nil)
    expect(ary).to eq([List[4,1,8],List[5,2,nil],List[6,nil,nil]])
    expect{a.zip("a")}.to raise_error  # <= 1.9.3 NoMethodError, >= 2.0.0 TypeError
  end

  it "transpose" do
    expect(List[].transpose).to eq(List[])
    expect(List[List[1,2],List[3,4],List[5,6]].transpose).to eq(List[List[1,3,5],List[2,4,6]])
    expect(List[[1,2],[3,4],[5,6]].transpose).to eq(List[List[1,3,5],List[2,4,6]])
    expect{List[[1],[2,3]].transpose}.to raise_error(IndexError)
    expect{List[1].transpose}.to raise_error(TypeError)
  end

  it "fill" do
    list = List["a","b","c","d"]
    expect(list.fill("x")).to eq(List["x","x","x","x"])
    expect(list.fill("z",2,2)).to eq(List["x","x","z","z"])
    expect(list.fill("y",0..1)).to eq(List["y","y","z","z"])
    expect(list.fill{"foo"}).to eq(List["foo","foo","foo","foo"])
    expect(list[0].object_id).to_not eq(list[1].object_id)
    expect(list.fill{|i| i*i}).to eq(List[0,1,4,9])
    expect(list.fill(-2){|i| i*i*i}).to eq(List[0,1,8,27])
    expect(list.fill("z",2)).to eq(List[0,1,"z","z"])
    expect{list.fill("z","a")}.to raise_error(TypeError)
  end

  it "include?" do
    list = List.new
    expect(list.include?(1)).to eq(false)
    list.push 1,2,3
    expect(list.include?(1)).to eq(true)
    expect(list.include?(10)).to eq(false)
  end

  it "<=>" do
    expect(List["a","a","c"] <=> List["a","b","c"]).to eq(-1)
    expect(List[1,2,3,4,5,6] <=> List[1,2]).to eq(+1)
    expect(List[1,2] <=> List[1,2]).to eq(0)
    expect(List[1,2] <=> List[1,:two]).to eq(nil)
  end

  it "slice" do
    list = List.new
    expect(list.slice(0)).to eq(nil)
    list.push 1,2,3,4,5
    expect(list.slice(0)).to eq(1)
    expect(list.slice(1,3)).to eq(List[2,3,4])
    expect(list.slice(1..3)).to eq(List[2,3,4])
  end

  it "slice!" do
    list = List.new
    expect(list.slice!(0)).to eq(nil)
    list.push 1,2,3,4,5
    expect(list.slice!(2)).to eq(3)
    expect(list.slice!(2,3)).to eq(List[4,5])
    expect(list.slice!(0..1)).to eq(List[1,2])
    list.push 1,2,3,4,5
    expect(list.slice!(-1)).to eq(5)
    expect(list.slice!(100..110)).to eq(nil)
    expect(list.slice!(100)).to eq(nil)
    expect(list.slice!(100,1)).to eq(nil)
    expect{list.slice!(0xffffffffffffffff)}.to raise_error(RangeError)
    expect{list.slice!("a")}.to raise_error(TypeError)
    expect{list.slice!(1,"a")}.to raise_error(TypeError)
    expect{list.slice!(1,2,3)}.to raise_error(ArgumentError)
    expect(list).to eq(List[1,2,3,4])
  end

  it "ring" do
    len = 0
    result = []
    list = List[1,2,3]
    ring = list.ring
    ring.each do |i|
      break if len == 1000
      result << i
      len += 1
    end
    expect(result).to eq([1,2,3] * 333 + [1])
    expect(ring.object_id).to_not eq(list.object_id)
    expect{ring.push 1}.to raise_error(RuntimeError)
  end

  it "ring!" do
    len = 0
    result = []
    list = List[1,2,3]
    ring = list.ring!
    ring.each do |i|
      break if len == 1000
      result << i
      len += 1
    end
    expect(result).to eq([1,2,3] * 333 + [1])
    expect(ring.object_id).to eq(list.object_id)
    expect{ring.push 1}.to raise_error(RuntimeError)
  end

  it "ring?" do
    list = List[1,2,3]
    expect(list.ring?).to eq(false)
    expect(list.ring.ring?).to eq(true)
    expect(list.ring!.ring?).to eq(true)
  end
end
