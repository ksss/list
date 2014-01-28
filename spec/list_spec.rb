require 'spec_helper'

describe LinkedList do
  before :each do
    GC.start
  end

  after :each do
    GC.start
  end

  it "class" do
    expect(LinkedList).to be_include(Enumerable)
  end

  it "create" do
    expect(LinkedList[]).to eq(LinkedList.new)
    expect(LinkedList[1,2,3]).to eq(LinkedList.new [1,2,3])
  end

  it "try_convert" do
    expect(LinkedList.try_convert LinkedList[1,2,3]).to eq(LinkedList[1,2,3])
    expect(LinkedList.try_convert [1,2,3]).to eq(LinkedList[1,2,3])
    expect(LinkedList.try_convert "[1,2,3]").to eq(nil)
  end

  it "to_list" do
    expect([].to_list).to eq(LinkedList.new)
    expect([1,[2],3].to_list).to eq(LinkedList[1,[2],3])
    expect((0..5).to_list).to eq(LinkedList[0,1,2,3,4,5])
  end

  it "initialize" do
    expect(LinkedList.new).to be_a_kind_of(LinkedList)
    expect(LinkedList.new([])).to be_a_kind_of(LinkedList)
    expect(LinkedList.new([1,2,3])).to be_a_kind_of(LinkedList)
  end

  it "dup and replace" do
    list = LinkedList.new
    expect(list.dup).to eq(list)
    list = LinkedList[1,2,3]
    expect(list.dup).to eq(list)
  end

  it "inspect" do
    list = LinkedList.new
    expect(list.inspect).to eq("#<LinkedList: []>")
    list.push 1,[2],{:a=>3}
    expect(list.inspect).to eq("#<LinkedList: [1, [2], {:a=>3}]>")
    expect(LL[1,2,3].inspect).to eq("#<LL: [1, 2, 3]>")
  end

  it "to_a" do
    list = LinkedList.new
    expect(list.to_a).to eq([])
    a = (0..10).to_a
    list = LinkedList.new(a)
    expect(list.to_a).to eq(a)
  end

  it "freeze and frozen?" do
    list = LinkedList.new
    list.freeze
    expect(list).to eq(LinkedList[])
    expect(list.frozen?).to eq(true)
    expect{ list.push 1 }.to raise_error
  end

  it "==" do
    list = LinkedList.new
    expect(list).to eq(LinkedList.new)
    expect(list).to eq(list)
    list.push(1)
    expect(list).to_not eq(LinkedList.new)
    expect(list).to eq(LinkedList[1])
    expect(list).to_not eq([1])
    expect(LL[1]).to eq(LL[1])
    expect(LL[1]).to eq(LinkedList[1])
  end

  it "eql?" do
    list = LinkedList.new
    expect(list.eql?(list)).to be true
    expect(list.eql?(LinkedList.new)).to be false
  end

  it "hash" do
    expect(LinkedList.new.hash).to eq(LinkedList.new.hash)
    list1 = LinkedList[1,2,3]
    list2 = LinkedList[1,2,3]
    expect(list1.hash).to eq(list2.hash)
    hash = {}
    hash[list1] = 1
    expect(hash[list1]).to eq(1)
  end

  it "[]" do
    list = LinkedList.new
    expect(list[0]).to eq(nil)
    list.push 1,2,3
    expect(list[2]).to eq(3)
    expect(list[-1]).to eq(3)
    expect(list[10]).to eq(nil)
    expect(list[1,1]).to eq(LinkedList[2])
    expect(list[1,2]).to eq(LinkedList[2,3])
    expect(list[1,10]).to eq(LinkedList[2,3])

    # special cases
    expect(list[3]).to eq(nil)
    expect(list[10,0]).to eq(nil)
    expect(list[3,0]).to eq(LinkedList[])
    expect(list[3..10]).to eq(LinkedList[])
  end

  it "[]=" do
    list = LinkedList.new
    list[0] = 1
    expect(list).to eq(LinkedList[1])
    list[1] = 2
    expect(list).to eq(LinkedList[1,2])
    list[4] = 4
    expect(list).to eq(LinkedList[1,2,nil,nil,4])
    list[0,3] = LinkedList['a','b','c']
    expect(list).to eq(LinkedList['a','b','c',nil,4])
    list[0,3] = LinkedList['a','b','c']
    expect(list).to eq(LinkedList['a','b','c',nil,4])
    list[1..2] = LinkedList[1,2]
    expect(list).to eq(LinkedList['a',1,2,nil,4])
    list[0,2] = "?"
    expect(list).to eq(LinkedList["?",2,nil,4])
    list[0..2] = "A"
    expect(list).to eq(LinkedList["A",4])
    list[-1] = "Z"
    expect(list).to eq(LinkedList["A","Z"])
    list[1..-1] = nil
    expect(list).to eq(LinkedList["A",nil])
    list[1..-1] = LinkedList[]
    expect(list).to eq(LinkedList["A"])
    list[0,0] = LinkedList[1,2]
    expect(list).to eq(LinkedList[1,2,"A"])
    list[3,0] = "B"
    expect(list).to eq(LinkedList[1,2,"A","B"])
    list[6,0] = "C"
    expect(list).to eq(LinkedList[1,2,"A","B",nil,nil,"C"])
    expect{list["10"]}.to raise_error(TypeError)

    a = LinkedList[1,2,3]
    a[1,0] = a
    expect(a).to eq(LinkedList[1,1,2,3,2,3]);

    a = LinkedList[1,2,3]
    a[-1,0] = a
    expect(a).to eq(LinkedList[1,2,1,2,3,3]);
  end

  it "at" do
    list = LinkedList.new
    expect(list.at(0)).to eq(nil)
    list.push 1,2,3
    expect(list.at(0)).to eq(1)
    expect(list.at(2)).to eq(3)
    expect(list.at(3)).to eq(nil)
    expect{list.at("10")}.to raise_error(TypeError)
  end

  it "fetch" do
    list = LinkedList[11,22,33,44]
    expect(list.fetch(1)).to eq(22)
    expect(list.fetch(-1)).to eq(44)
    expect(list.fetch(4, 'cat')).to eq('cat')
    expect(list.fetch(100){|i| "#{i} is out of bounds" }).to eq('100 is out of bounds')
    expect{list.fetch(100)}.to raise_error(IndexError)
    expect{list.fetch("100")}.to raise_error(TypeError)
  end

  it "first" do
    list = LinkedList.new
    expect(list.first).to eq(nil)
    list.push 1,2,3
    expect(list.first).to eq(1)
    expect(list.first).to eq(1)
    expect(list.first(2)).to eq(LinkedList[1,2])
    expect(list.first(10)).to eq(LinkedList[1,2,3])
    expect{list.first("1")}.to raise_error(TypeError)
    expect{list.first(-1)}.to raise_error(ArgumentError)
  end

  it "last" do
    list = LinkedList.new
    expect(list.last).to eq(nil)
    list.push 1,2,3
    expect(list.last).to eq(3)
    expect(list.last).to eq(3)
    expect(list.last(2)).to eq(LinkedList[2,3])
    expect(list.last(10)).to eq(LinkedList[1,2,3])
    expect{list.last("1")}.to raise_error(TypeError)
    expect{list.last(-1)}.to raise_error(ArgumentError)
  end

  it "concat" do
    list = LinkedList.new
    list2 = LinkedList[1,2,3]
    expect(list.concat(list2)).to eq(LinkedList[1,2,3])
    expect(list.concat([4,5,6])).to eq(LinkedList[1,2,3,4,5,6])
    expect(list.concat(LL[7,8,9])).to eq(LinkedList[1,2,3,4,5,6,7,8,9])
  end

  it "push" do
    list = LinkedList.new
    expect(list.push).to eq(list)
    10.times { |i|
      list << i
    }
    expect(list).to eq((0...10).to_list)
    list.push(*(10...20).to_a)
    expect(list).to eq((0...20).to_list)
  end

  it "pop" do
    list = LinkedList.new
    expect(list.pop).to eq(nil)
    list.push 1,2,3
    expect(list.pop(2).to_a).to eq([2,3])
    expect(list.pop).to eq(1)
    expect{list.pop("1")}.to raise_error(TypeError) 
  end

  it "shift" do
    list = LinkedList.new
    expect(list.shift).to eq(nil)
    list.push 1,2,3
    expect(list.shift(2).to_a).to eq([1,2])
    expect(list.shift).to eq(3)
    expect{list.shift("1")}.to raise_error(TypeError) 
  end

  it "unshift" do
    list = LinkedList.new
    expect(list.unshift).to eq(list)
    expect(list.unshift(3).to_a).to eq([3])
    expect(list.unshift(2,1).to_a).to eq([1,2,3])
  end

  it "insert" do
    list = LinkedList['a','b','c','d']
    expect(list.insert(2, 99)).to eq(LinkedList["a","b",99,"c","d"]);
    expect{list.insert("a", 1)}.to raise_error(TypeError)
  end

  it "each" do
    list = LinkedList.new
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
    list = LinkedList.new
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
    list = LinkedList[1,2,3]
    result = []
    list.reverse_each do |i|
      result << i
    end
    expect(result).to eq([3,2,1])
  end

  it "length" do
    list = LinkedList.new
    expect(list.length).to eq(0)
    list.push 1,2,3
    expect(list.length).to eq(3)
    expect(list.size).to eq(3)
  end

  it "empty?" do
    list = LinkedList.new
    expect(list.empty?).to eq(true)
    list.push 1,2,3
    expect(list.empty?).to eq(false)
  end

  it "find_index" do
    list = LinkedList.new
    expect(list.find_index(1)).to eq(nil)
    list.push 1,2,3
    expect(list.find_index(1)).to eq(0)
    expect(list.find_index(2)).to eq(1)
    expect(list.index(3)).to eq(2)
  end

  it "replace" do
    list = LinkedList[1,2,3,4,5]
    expect(list.replace(LinkedList[4,5,6])).to eq(LinkedList[4,5,6])
    expect(list.replace([7,8,9])).to eq(LinkedList[7,8,9])
    expect{list.replace "1"}.to raise_error(TypeError)
  end

  it "clear" do
    list = LinkedList[1,2,3]
    expect(list.clear).to eq(LinkedList[])
    expect(list.clear).to eq(LinkedList[])
  end

  it "rindex" do
    list = LinkedList[1,2,2,2,3]
    expect(list.rindex(2)).to eq(3)
    expect(list.rindex(-10)).to eq(nil)
    expect(list.rindex { |x| x == 2 }).to eq(3)
  end

  it "join" do
    expect(LinkedList.new.join).to eq("")
    expect(LinkedList[1,2,3].join).to eq("123")
    expect(LinkedList["a","b","c"].join).to eq("abc")
    expect(LinkedList["a","b","c"].join("-")).to eq("a-b-c")
    expect(LinkedList["a",LinkedList["b",LinkedList["c"]]].join("-")).to eq("a-b-c")
    expect{LinkedList["a","b","c"].join(1)}.to raise_error(TypeError)
    orig_sep = $,
    $, = "*"
    expect(LinkedList["a","b","c"].join).to eq("a*b*c")
    $, = orig_sep
  end

  it "reverse" do
    list = LinkedList.new
    expect(list.reverse).to eq(LinkedList.new)
    list.push 1,2,3
    expect(list.reverse).to eq(LinkedList[3,2,1])
    expect(list).to eq(LinkedList[1,2,3])
  end

  it "reverse!" do
    list = LinkedList.new
    expect(list.reverse!).to eq(LinkedList.new)
    list.push 1,2,3
    expect(list.reverse!).to eq(LinkedList[3,2,1])
    expect(list).to eq(LinkedList[3,2,1])
  end

  it "rotate" do
    list = LinkedList.new
    expect(list.rotate).to eq(LinkedList.new)
    list.push 1,2,3
    expect(list.rotate).to eq(LinkedList[2,3,1])
    expect(list.rotate(2)).to eq(LinkedList[3,1,2])
    expect(list.rotate(-2)).to eq(LinkedList[2,3,1])
    expect(list).to eq(LinkedList[1,2,3])
    expect{list.rotate("a")}.to raise_error(TypeError)
    expect{list.rotate(1,2)}.to raise_error(ArgumentError)
  end

  it "rotate!" do
    list = LinkedList.new
    expect(list.rotate!).to eq(LinkedList.new)
    list.push 1,2,3
    expect(list.rotate!).to eq(LinkedList[2,3,1])
    expect(list.rotate!(2)).to eq(LinkedList[1,2,3])
    expect(list.rotate!(-2)).to eq(LinkedList[2,3,1])
    expect(list).to eq(LinkedList[2,3,1])
    expect{list.rotate!("a")}.to raise_error(TypeError)
    expect{list.rotate!(1,2)}.to raise_error(ArgumentError)
  end

  it "sort" do
    list = LinkedList.new
    expect(list.sort).to eq(LinkedList.new)
    list.push *[4,1,3,5,2]
    expect(list.sort).to eq(LinkedList[1,2,3,4,5])
    expect(list.sort{|a,b| b - a}).to eq(LinkedList[5,4,3,2,1])
    expect(list).to eq(LinkedList[4,1,3,5,2])
  end

  it "sort!" do
    list = LinkedList.new
    expect(list.sort!).to eq(LinkedList.new)
    list.push *[4,1,3,5,2]
    expect(list.sort!).to eq(LinkedList[1,2,3,4,5])
    expect(list).to eq(LinkedList[1,2,3,4,5])
    expect(list.sort!{|a,b| b - a}).to eq(LinkedList[5,4,3,2,1])
    expect(list).to eq(LinkedList[5,4,3,2,1])
  end

  it "sort_by" do
    list = LinkedList.new
    expect(list.sort_by{|a| a}).to eq(LinkedList.new)
    list.push *[4,1,3,5,2]
    expect(list.sort_by{|a| a}).to eq(LinkedList[1,2,3,4,5])
    expect(list.sort_by{|a| -a}).to eq(LinkedList[5,4,3,2,1])
    expect(list).to eq(LinkedList[4,1,3,5,2])
  end

  it "sort_by!" do
    list = LinkedList.new
    expect(list.sort_by!{|a| a}).to eq(LinkedList.new)
    list.push *[4,1,3,5,2]
    expect(list.sort_by!{|a| a}).to eq(LinkedList[1,2,3,4,5])
    expect(list.sort_by!{|a| -a}).to eq(LinkedList[5,4,3,2,1])
    expect(list).to eq(LinkedList[5,4,3,2,1])
  end

  it "collect" do
    list = LinkedList.new
    expect(list.collect{|a| a}).to eq(LinkedList.new)
    list.push *[4,1,3,5,2]
    expect(list.collect{|a| a * a}).to eq(LinkedList[16,1,9,25,4])
    expect(list.map.each{|a| -a}).to eq(LinkedList[-4,-1,-3,-5,-2])
    expect(list).to eq(LinkedList[4,1,3,5,2])
  end

  it "collect!" do
    list = LinkedList.new
    expect(list.collect!{|a| a}).to eq(LinkedList.new)
    list.push *[4,1,3,5,2]
    expect(list.collect!{|a| a * a}).to eq(LinkedList[16,1,9,25,4])
    expect(list.map!.each{|a| -a}).to eq(LinkedList[-16,-1,-9,-25,-4])
  end

  it "select" do
    list = LinkedList.new
    expect(list.select{|i| i}).to eq(LinkedList.new)
    list.push *[4,1,3,5,2]
    expect(list.select{|i| i.even?}).to eq(LinkedList[4,2])
    expect(list.select.each{|i| i.odd?}).to eq(LinkedList[1,3,5])
    expect(list).to eq(LinkedList[4,1,3,5,2])
  end

  it "select!" do
    list = LinkedList.new
    expect(list.select!{|i| i}).to eq(nil)
    list.push *[4,1,3,5,2]
    expect(list.select!{|i| i.even?}).to eq(LinkedList[4,2])
    expect(list.select!.each{|i| i % 4 == 0}).to eq(LinkedList[4])
    expect(list.select!.each{|i| i % 4 == 0}).to eq(nil)
    expect(list).to eq(LinkedList[4])
  end

  it "keep_if" do
    list = LinkedList.new
    expect(list.keep_if{|i| i}).to eq(list)
    list.push *[4,1,3,5,2]
    expect(list.keep_if{|i| i.even?}).to eq(LinkedList[4,2])
    expect(list.keep_if.each{|i| i % 4 == 0}).to eq(LinkedList[4])
    expect(list.keep_if.each{|i| i % 4 == 0}).to eq(list)
    expect(list).to eq(LinkedList[4])
  end

  it "values_at" do
    expect(LinkedList.new.values_at(1)).to eq(LinkedList[nil])
    list = LinkedList[4,1,3,5,2]
    expect(list.values_at).to eq(LinkedList.new)
    expect(list.values_at 1).to eq(LinkedList[1])
    expect(list.values_at 1,3,10).to eq(LinkedList[1,5,nil])
    expect(list.values_at -1,-2).to eq(LinkedList[2,5])
    expect(list.values_at 1..5).to eq(LinkedList[1,3,5,2,nil])
    expect(list.values_at -2..-1).to eq(LinkedList[5,2])
    expect(list.values_at -1..-2).to eq(LinkedList.new)
    expect(list.values_at 0,1..5).to eq(LinkedList[4,1,3,5,2,nil])
    expect(list.values_at 10..12).to eq(LinkedList[nil,nil,nil])
    expect{list.values_at "a"}.to raise_error(TypeError)
  end

  it "delete" do
    expect(LinkedList.new.delete(nil)).to eq(nil)
    list = LinkedList[4,1,3,2,5,5]
    expect(list.delete(1)).to eq(1)
    expect(list).to eq(LinkedList[4,3,2,5,5])
    expect(list.delete(5){"not found"}).to eq(5)
    expect(list).to eq(LinkedList[4,3,2])
    expect(list.delete(4)).to eq(4)
    expect(list.delete(1){"not found"}).to eq("not found")

    list = Array.new(1024,0).to_list
    expect(list.delete(0)).to eq(0)
    expect(list).to eq(LinkedList.new)
  end

  it "delete_at" do
    list = LinkedList[1,2,3]
    expect(list.delete_at(1)).to eq(2)
    expect(list).to eq(LinkedList[1,3])
    expect(list.delete_at(-1)).to eq(3)
    expect(list).to eq(LinkedList[1])
    expect(list.delete_at(0)).to eq(1)
    expect(list).to eq(LinkedList[])
    expect(list.delete_at(0)).to eq(nil)
    expect{list.delete_at("a")}.to raise_error(TypeError)
  end

  it "delete_if" do
    list = LinkedList[1,2,3,4,5]
    expect(list.delete_if{|i| 3 < i}).to eq(LinkedList[1,2,3])
    expect(list.delete_if.each{|i| i.even?}).to eq(LinkedList[1,3])
    expect(list.delete_if{|i| 0 < i}).to eq(LinkedList[])
    expect(list.delete_if{|i| 0 < i}).to eq(LinkedList[])
    expect(list).to eq(LinkedList[])
  end

  it "reject" do
    list = LinkedList[1,2,3,4,5]
    expect(list.reject{|i| 3 < i}).to eq(LinkedList[1,2,3])
    expect(list.reject.each{|i| i.even?}).to eq(LinkedList[1,3,5])
    expect(list.reject{|i| 0 < i}).to eq(LinkedList[])
    expect(list).to eq(LinkedList[1,2,3,4,5])
  end

  it "reject!" do
    list = LinkedList[1,2,3,4,5]
    expect(list.reject!{|i| i % 10 == 0}).to eq(nil)
    expect(list.reject!{|i| 3 < i}).to eq(LinkedList[1,2,3])
    expect(list.reject!.each{|i| i.even?}).to eq(LinkedList[1,3])
    expect(list.reject!{|i| 0 < i}).to eq(LinkedList[])
    expect(list.reject!{|i| 0 < i}).to eq(nil)
    expect(list).to eq(LinkedList[])
  end

  it "zip" do
    a = LinkedList[4,5,6]
    b = LinkedList[7,8,9]
    ary = []
    expect(a.zip(b)).to eq(LinkedList[LinkedList[4,7],LinkedList[5,8],LinkedList[6,9]])
    expect(LinkedList[1,2,3].zip(a,b)).to eq(LinkedList[LinkedList[1,4,7],LinkedList[2,5,8],LinkedList[3,6,9]])
    expect(LinkedList[1,2].zip(a,b)).to eq(LinkedList[LinkedList[1,4,7],LinkedList[2,5,8]])
    expect(a.zip(LinkedList[1,2],LinkedList[8])).to eq(LinkedList[LinkedList[4,1,8],LinkedList[5,2,nil],LinkedList[6,nil,nil]])
    expect(a.zip([1,2],[8])).to eq(LinkedList[LinkedList[4,1,8],LinkedList[5,2,nil],LinkedList[6,nil,nil]])
    expect(a.zip([1,2],[8]){|i| ary << i}).to eq(nil)
    expect(ary).to eq([LinkedList[4,1,8],LinkedList[5,2,nil],LinkedList[6,nil,nil]])
    expect{a.zip("a")}.to raise_error  # <= 1.9.3 NoMethodError, >= 2.0.0 TypeError
  end

  it "transpose" do
    expect(LinkedList[].transpose).to eq(LinkedList[])
    expect(LinkedList[LinkedList[1,2],LinkedList[3,4],LinkedList[5,6]].transpose).to eq(LinkedList[LinkedList[1,3,5],LinkedList[2,4,6]])
    expect(LinkedList[[1,2],[3,4],[5,6]].transpose).to eq(LinkedList[LinkedList[1,3,5],LinkedList[2,4,6]])
    expect{LinkedList[[1],[2,3]].transpose}.to raise_error(IndexError)
    expect{LinkedList[1].transpose}.to raise_error(TypeError)
  end

  it "fill" do
    list = LinkedList["a","b","c","d"]
    expect(list.fill("x")).to eq(LinkedList["x","x","x","x"])
    expect(list.fill("z",2,2)).to eq(LinkedList["x","x","z","z"])
    expect(list.fill("y",0..1)).to eq(LinkedList["y","y","z","z"])
    expect(list.fill{"foo"}).to eq(LinkedList["foo","foo","foo","foo"])
    expect(list[0].object_id).to_not eq(list[1].object_id)
    expect(list.fill{|i| i*i}).to eq(LinkedList[0,1,4,9])
    expect(list.fill(-2){|i| i*i*i}).to eq(LinkedList[0,1,8,27])
    expect(list.fill("z",2)).to eq(LinkedList[0,1,"z","z"])
    expect{list.fill("z","a")}.to raise_error(TypeError)
  end

  it "include?" do
    list = LinkedList.new
    expect(list.include?(1)).to eq(false)
    list.push 1,2,3
    expect(list.include?(1)).to eq(true)
    expect(list.include?(10)).to eq(false)
  end

  it "<=>" do
    expect(LinkedList["a","a","c"] <=> LinkedList["a","b","c"]).to eq(-1)
    expect(LinkedList[1,2,3,4,5,6] <=> LinkedList[1,2]).to eq(+1)
    expect(LinkedList[1,2] <=> LinkedList[1,2]).to eq(0)
    expect(LinkedList[1,2] <=> LinkedList[1,:two]).to eq(nil)
  end

  it "slice" do
    list = LinkedList.new
    expect(list.slice(0)).to eq(nil)
    list.push 1,2,3,4,5
    expect(list.slice(0)).to eq(1)
    expect(list.slice(1,3)).to eq(LinkedList[2,3,4])
    expect(list.slice(1..3)).to eq(LinkedList[2,3,4])
  end

  it "slice!" do
    list = LinkedList.new
    expect(list.slice!(0)).to eq(nil)
    list.push 1,2,3,4,5
    expect(list.slice!(2)).to eq(3)
    expect(list.slice!(2,3)).to eq(LinkedList[4,5])
    expect(list.slice!(0..1)).to eq(LinkedList[1,2])
    list.push 1,2,3,4,5
    expect(list.slice!(-1)).to eq(5)
    expect(list.slice!(100..110)).to eq(nil)
    expect(list.slice!(100)).to eq(nil)
    expect(list.slice!(100,1)).to eq(nil)
    expect{list.slice!(0xffffffffffffffff)}.to raise_error(RangeError)
    expect{list.slice!("a")}.to raise_error(TypeError)
    expect{list.slice!(1,"a")}.to raise_error(TypeError)
    expect{list.slice!(1,2,3)}.to raise_error(ArgumentError)
    expect(list).to eq(LinkedList[1,2,3,4])
  end

  it "ring" do
    len = 0
    result = []
    list = LinkedList[1,2,3]
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
    list = LinkedList[1,2,3]
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
    list = LinkedList[1,2,3]
    expect(list.ring?).to eq(false)
    expect(list.ring.ring?).to eq(true)
    expect(list.ring!.ring?).to eq(true)
  end
end
