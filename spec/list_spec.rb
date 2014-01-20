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
  end

  it "try_convert" do
    expect(List.try_convert List[1,2,3]).to eq(List[1,2,3])
    expect(List.try_convert [1,2,3]).to eq(List[1,2,3])
    expect(List.try_convert "[1,2,3]").to eq(nil)
  end

  it "initialize" do
    expect(List.new).to be_a_kind_of(List)
    expect(List.new([])).to be_a_kind_of(List)
    expect(List.new([1])).to be_a_kind_of(List)
  end

  it "dup and replace" do
    list = List.new
    expect(list.dup.to_a).to eq(list.to_a)
    list = List[1,2,3]
    expect(list.dup.to_a).to eq(list.to_a)
  end

  it "inspect" do
    list = List.new
    expect(list.inspect).to eq("#<List: []>")
    list.push 1,[2],{:a=>3}
    expect(list.inspect).to eq("#<List: [1, [2], {:a=>3}]>")
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
    expect(list.to_a).to eq([])
    expect(list.frozen?).to eq(true)
    expect{ list.push 1 }.to raise_error
  end

  it "==" do
    list = List.new
    expect(list).to eq(List.new)
    list.push(1)
    expect(list).to_not eq(List.new)
    expect(list.to_a).to eq([1])
    expect(list).to_not eq([1])
  end

  it "eql?" do
    list = List.new
    expect(list.eql?(list)).to be true
    expect(list.eql?(List.new)).to be false
  end

  it "hash" do
    expect(List.new.hash).to eq(List.new.hash)
    list1 = List[1,2,3]
    list2 = List[1,2,3]
    expect(list1.hash).to eq(list2.hash)
  end

  it "[]" do
    list = List.new
    expect(list[0]).to eq(nil)
    list.push 1,2,3
    expect(list[2]).to eq(3)
    expect(list[-1]).to eq(3)
    expect(list[10]).to eq(nil)
    expect(list[1,1].to_a).to eq([2])
    expect(list[1,2].to_a).to eq([2,3])
    expect(list[1,10].to_a).to eq([2,3])

    # special cases
    expect(list[3]).to eq(nil)
    expect(list[10,0]).to eq(nil)
    expect(list[3,0].to_a).to eq([])
    expect(list[3..10].to_a).to eq([])
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
    expect(list.concat(list2).to_a).to eq([1,2,3])
    expect(list.concat([4,5,6]).to_a).to eq([1,2,3,4,5,6])
  end

  it "push" do
    list = List.new
    expect(list.push).to eq(list)
    10.times { |i|
      list << i
    }
    expect(list.to_a).to eq((0...10).to_a)
    list.push(*(10...20).to_a)
    expect(list.to_a).to eq((0...20).to_a)
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
  end

  it "replace" do
    list = List[1,2,3,4,5]
    expect(list.replace(List[4,5,6]).to_a).to eq([4,5,6])
    expect(list.replace([7,8,9]).to_a).to eq([7,8,9])
    expect{list.replace "1"}.to raise_error(TypeError)
  end

  it "clear" do
    list = List[1,2,3]
    expect(list.clear.to_a).to eq([])
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
