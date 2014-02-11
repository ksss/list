require 'spec_helper'

describe List do
  before :all do
    @cls = List
    @subcls = L
  end

  before :each do
    GC.start
  end

  after :each do
    GC.start
  end

  it "class" do
    expect(@cls).to be_include(Enumerable)
  end

  it "create" do
    expect(@cls[]).to eq(@cls.new)
    expect(@cls[1,2,3]).to eq(@cls.new [1,2,3])
    expect(@subcls[1,2,3]).to eq(@subcls.new [1,2,3])
  end

  it "try_convert" do
    expect(@cls.try_convert @cls[1,2,3]).to eq(@cls[1,2,3])
    expect(@cls.try_convert [1,2,3]).to eq(@cls[1,2,3])
    expect(@cls.try_convert "[1,2,3]").to eq(nil)
  end

  it "to_list" do
    expect([].to_list).to eq(@cls.new)
    expect([1,[2],3].to_list).to eq(@cls[1,[2],3])
    expect((0..5).to_list).to eq(@cls[0,1,2,3,4,5])
    expect(@cls[1,2,3].to_list).to eq(@cls[1,2,3])
    expect(@subcls[1,2,3].to_list).to eq(@subcls[1,2,3])
    klass = Class.new(Array)
    list = klass.new.to_list
    expect(list).to eq(@cls[])
    expect(list.class).to eq(@cls)
  end

  it "initialize" do
    expect(@cls.new).to be_a_kind_of(@cls)
    expect(@subcls.new).to be_a_kind_of(@cls)
    expect(@cls.new([])).to be_a_kind_of(@cls)
    expect(@cls.new([1,2,3])).to be_a_kind_of(@cls)
    expect(@cls.new([1,2,3])).to eq(@cls[1,2,3])
    expect(@cls.new(3)).to eq(@cls[nil,nil,nil])
    expect(@cls.new(3, 0)).to eq(@cls[0,0,0])
    expect(@cls.new(3){|i| "foo"}).to eq(@cls["foo", "foo", "foo"])
    expect{@cls.new.instance_eval{ initialize }}.to_not raise_error
    expect{@cls.new {}}.to_not raise_error
    expect{@cls.new(3,0,0)}.to raise_error(ArgumentError)
    expect{@cls.new(-1,1)}.to raise_error(ArgumentError)
    expect{@cls.new("a")}.to raise_error(TypeError)
    expect{@cls.new("a",0)}.to raise_error(TypeError)
  end

  it "dup and replace" do
    list = @cls.new
    expect(list.dup).to eq(list)
    list = @cls[1,2,3]
    expect(list.dup).to eq(list)
  end

  it "inspect" do
    list = @cls.new
    expect(list.inspect).to eq("#<#{@cls}: []>")
    list.push 1,[2],{:a=>3}
    expect(list.inspect).to eq("#<#{@cls}: [1, [2], {:a=>3}]>")
    expect(@subcls[1,2,3].inspect).to eq("#<#{@subcls}: [1, 2, 3]>")
  end

  it "to_a" do
    list = @cls.new
    expect(list.to_a).to eq([])
    a = (0..10).to_a
    list = @cls.new(a)
    expect(list.to_a).to eq(a)
  end

  it "freeze and frozen?" do
    list = @cls.new
    list.freeze
    expect(list).to eq(@cls[])
    expect(list.frozen?).to eq(true)
    expect{ list.push 1 }.to raise_error
  end

  it "==" do
    list = @cls.new
    expect(list).to eq(@cls.new)
    expect(list).to eq(list)
    list.push(1)
    expect(list).to_not eq(@cls.new)
    expect(list).to eq(@cls[1])
    expect(list).to_not eq([1])
    expect(@subcls[1]).to eq(@subcls[1])
    expect(@subcls[1]).to eq(@cls[1])
  end

  it "eql?" do
    list = @cls.new
    expect(list.eql?(list)).to be true
    expect(list.eql?(@cls.new)).to be false
    expect(list.eql?(@subcls.new)).to be false
  end

  it "hash" do
    expect(@cls.new.hash).to eq(@cls.new.hash)
    list1 = @cls[1,2,3]
    list2 = @cls[1,2,3]
    expect(list1.hash).to eq(list2.hash)
    hash = {}
    hash[list1] = 1
    expect(hash[list1]).to eq(1)
  end

  it "[]" do
    list = @cls.new
    expect(list[0]).to eq(nil)
    list.push 1,2,3
    expect(list[2]).to eq(3)
    expect(list[-1]).to eq(3)
    expect(list[10]).to eq(nil)
    expect(list[1,1]).to eq(@cls[2])
    expect(list[1,2]).to eq(@cls[2,3])
    expect(list[1,10]).to eq(@cls[2,3])

    # special cases
    expect(list[3]).to eq(nil)
    expect(list[10,0]).to eq(nil)
    expect(list[3,0]).to eq(@cls[])
    expect(list[3..10]).to eq(@cls[])

    expect{@cls[][0,0,0]}.to raise_error(ArgumentError)
  end

  it "[]=" do
    list = @cls.new
    list[0] = 1
    expect(list).to eq(@cls[1])
    list[1] = 2
    expect(list).to eq(@cls[1,2])
    list[4] = 4
    expect(list).to eq(@cls[1,2,nil,nil,4])
    list[0,3] = @cls['a','b','c']
    expect(list).to eq(@cls['a','b','c',nil,4])
    list[0,3] = @cls['a','b','c']
    expect(list).to eq(@cls['a','b','c',nil,4])
    list[1..2] = @cls[1,2]
    expect(list).to eq(@cls['a',1,2,nil,4])
    list[0,2] = "?"
    expect(list).to eq(@cls["?",2,nil,4])
    list[0..2] = "A"
    expect(list).to eq(@cls["A",4])
    list[-1] = "Z"
    expect(list).to eq(@cls["A","Z"])
    list[1..-1] = nil
    expect(list).to eq(@cls["A",nil])
    list[1..-1] = @cls[]
    expect(list).to eq(@cls["A"])
    list[0,0] = @cls[1,2]
    expect(list).to eq(@cls[1,2,"A"])
    list[3,0] = "B"
    expect(list).to eq(@cls[1,2,"A","B"])
    list[6,0] = "C"
    expect(list).to eq(@cls[1,2,"A","B",nil,nil,"C"])

    a = @cls[1,2,3]
    a[1,0] = a
    expect(a).to eq(@cls[1,1,2,3,2,3]);

    a = @cls[1,2,3]
    a[-1,0] = a
    expect(a).to eq(@cls[1,2,1,2,3,3]);

    expect{@cls[0][-2] = 1}.to raise_error(IndexError)
    expect{@cls[0][-2,0] = nil}.to raise_error(IndexError)
    expect{@cls[0][0,0,0] = 0}.to raise_error(ArgumentError)
    expect{@cls[0].freeze[0,0,0] = 0}.to raise_error(ArgumentError)
    expect{@cls[0][:foo] = 0}.to raise_error(TypeError)
    expect{@cls[0].freeze[:foo] = 0}.to raise_error(RuntimeError)
  end

  it "at" do
    list = @cls.new
    expect(list.at(0)).to eq(nil)
    list.push 1,2,3
    expect(list.at(0)).to eq(1)
    expect(list.at(2)).to eq(3)
    expect(list.at(3)).to eq(nil)
    expect{list.at("10")}.to raise_error(TypeError)
  end

  it "fetch" do
    list = @cls[11,22,33,44]
    expect(list.fetch(1)).to eq(22)
    expect(list.fetch(-1)).to eq(44)
    expect(list.fetch(4, 'cat')).to eq('cat')
    expect(list.fetch(100){|i| "#{i} is out of bounds" }).to eq('100 is out of bounds')

    expect{@cls[0,1].fetch(100)}.to raise_error(IndexError)
    expect{@cls[0,1].fetch(-100)}.to raise_error(IndexError)
    expect{@cls[0,1].fetch("100")}.to raise_error(TypeError)
  end

  it "first" do
    list = @cls.new
    expect(list.first).to eq(nil)
    list.push 1,2,3
    expect(list.first).to eq(1)
    expect(list.first).to eq(1)
    expect(list.first(2)).to eq(@cls[1,2])
    expect(list.first(10)).to eq(@cls[1,2,3])
    expect{list.first("1")}.to raise_error(TypeError)
    expect{list.first(-1)}.to raise_error(ArgumentError)
  end

  it "last" do
    list = @cls.new
    expect(list.last).to eq(nil)
    list.push 1,2,3
    expect(list.last).to eq(3)
    expect(list.last).to eq(3)
    expect(list.last(2)).to eq(@cls[2,3])
    expect(list.last(10)).to eq(@cls[1,2,3])
    expect{list.last("1")}.to raise_error(TypeError)
    expect{list.last(-1)}.to raise_error(ArgumentError)
  end

  it "concat" do
    list = @cls.new
    list2 = @cls[1,2,3]
    expect(list.concat(list2)).to eq(@cls[1,2,3])
    expect(list.concat([4,5,6])).to eq(@cls[1,2,3,4,5,6])
    expect(list.concat(@subcls[7,8,9])).to eq(@cls[1,2,3,4,5,6,7,8,9])
  end

  it "push" do
    list = @cls.new
    expect(list.push).to eq(list)
    10.times { |i|
      list << i
    }
    expect(list).to eq(@cls[*0...10])
    list.push(*(10...20))
    expect(list).to eq(@cls[*0...20])
  end

  it "pop" do
    list = @cls.new
    expect(list.pop).to eq(nil)
    list.push 1,2,3
    expect(list.pop(2)).to eq(@cls[2,3])
    expect(list.pop).to eq(1)
    expect{list.pop("1")}.to raise_error(TypeError) 
  end

  it "shift" do
    list = @cls.new
    expect(list.shift).to eq(nil)
    list.push 1,2,3
    expect(list.shift(2)).to eq(@cls[1,2])
    expect(list.shift).to eq(3)
    expect{list.shift("1")}.to raise_error(TypeError) 
  end

  it "unshift" do
    list = @cls.new
    expect(list.unshift).to eq(list)
    expect(list.unshift(3)).to eq(@cls[3])
    expect(list.unshift(1,2)).to eq(@cls[1,2,3])
    expect(list.unshift(@cls["foo","bar"])).to eq(@cls[@cls["foo","bar"],1,2,3])

    expect{@cls[].freeze.unshift('cat')}.to raise_error(RuntimeError)
    expect{@cls[].freeze.unshift()}.to raise_error(RuntimeError)
  end

  it "insert" do
    list = @cls['a','b','c','d']
    expect(list.insert(2, 99)).to eq(@cls["a","b",99,"c","d"]);
    expect{list.insert("a", 1)}.to raise_error(TypeError)
  end

  it "each" do
    list = @cls.new
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
    list = @cls.new
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
    list = @cls[1,2,3]
    result = []
    list.reverse_each do |i|
      result << i
    end
    expect(result).to eq([3,2,1])
  end

  it "length" do
    list = @cls.new
    expect(list.length).to eq(0)
    list.push 1,2,3
    expect(list.length).to eq(3)
    expect(list.size).to eq(3)
  end

  it "empty?" do
    list = @cls.new
    expect(list.empty?).to eq(true)
    list.push 1,2,3
    expect(list.empty?).to eq(false)
  end

  it "find_index" do
    list = @cls.new
    expect(list.find_index(1)).to eq(nil)
    list.push 1,2,3
    expect(list.find_index(1)).to eq(0)
    expect(list.find_index(2)).to eq(1)
    expect(list.index(3)).to eq(2)
  end

  it "replace" do
    list = @cls[1,2,3,4,5]
    expect(list.replace(@cls[4,5,6])).to eq(@cls[4,5,6])
    expect(list.replace([7,8,9])).to eq(@cls[7,8,9])
    expect{list.replace "1"}.to raise_error(TypeError)
  end

  it "clear" do
    list = @cls[1,2,3]
    expect(list.clear).to eq(@cls[])
    expect(list.clear).to eq(@cls[])
  end

  it "rindex" do
    list = @cls[1,2,2,2,3]
    expect(list.rindex(2)).to eq(3)
    expect(list.rindex(-10)).to eq(nil)
    expect(list.rindex { |x| x == 2 }).to eq(3)

    list = @cls[0,1]
    e = list.rindex
    expect(e.next).to eq(1)
    list.clear
    expect{e.next}.to raise_error(StopIteration)

    obj = Object.new
    class << obj; self; end.class_eval do
      define_method(:==) {|x| list.clear; false}
    end
    list = @cls[nil,obj]
    expect(list.rindex(0)).to eq(nil)
  end

  it "join" do
    expect(@cls.new.join).to eq("")
    expect(@cls[1,2,3].join).to eq("123")
    expect(@cls["a","b","c"].join).to eq("abc")
    expect(@cls["a","b","c"].join("-")).to eq("a-b-c")
    expect(@cls["a",@cls["b",@cls["c"]]].join("-")).to eq("a-b-c")
    expect(@cls["a",["b",@cls[["c"]]]].join("-")).to eq("a-b-c")
    expect{@cls["a","b","c"].join(1)}.to raise_error(TypeError)
    orig_sep = $,
    $, = "*"
    expect(@cls["a","b","c"].join).to eq("a*b*c")
    $, = orig_sep

    def (a = Object.new).to_list
      List[self]
    end
    expect{@cls[a].join}.to raise_error(ArgumentError)
  end

  it "reverse" do
    list = @cls.new
    expect(list.reverse).to eq(@cls.new)
    list.push 1,2,3
    expect(list.reverse).to eq(@cls[3,2,1])
    expect(list).to eq(@cls[1,2,3])
  end

  it "reverse!" do
    list = @cls.new
    expect(list.reverse!).to eq(@cls.new)
    list.push 1,2,3
    expect(list.reverse!).to eq(@cls[3,2,1])
    expect(list).to eq(@cls[3,2,1])
  end

  it "rotate" do
    list = @cls.new
    expect(list.rotate).to eq(@cls.new)
    list.push 1,2,3
    expect(list.rotate).to eq(@cls[2,3,1])
    expect(list.rotate(2)).to eq(@cls[3,1,2])
    expect(list.rotate(-2)).to eq(@cls[2,3,1])
    expect(list).to eq(@cls[1,2,3])
    expect{list.rotate("a")}.to raise_error(TypeError)
    expect{list.rotate(1,2)}.to raise_error(ArgumentError)
  end

  it "rotate!" do
    list = @cls.new
    expect(list.rotate!).to eq(@cls.new)
    list.push 1,2,3
    expect(list.rotate!).to eq(@cls[2,3,1])
    expect(list.rotate!(2)).to eq(@cls[1,2,3])
    expect(list.rotate!(-2)).to eq(@cls[2,3,1])
    expect(list).to eq(@cls[2,3,1])
    expect{list.rotate!("a")}.to raise_error(TypeError)
    expect{list.rotate!(1,2)}.to raise_error(ArgumentError)
  end

  it "sort" do
    list = @cls.new
    expect(list.sort).to eq(@cls.new)
    list.push *[4,1,3,5,2]
    expect(list.sort).to eq(@cls[1,2,3,4,5])
    expect(list.sort{|a,b| b - a}).to eq(@cls[5,4,3,2,1])
    expect(list).to eq(@cls[4,1,3,5,2])
  end

  it "sort!" do
    list = @cls.new
    expect(list.sort!).to eq(@cls.new)
    list.push *[4,1,3,5,2]
    expect(list.sort!).to eq(@cls[1,2,3,4,5])
    expect(list).to eq(@cls[1,2,3,4,5])
    expect(list.sort!{|a,b| b - a}).to eq(@cls[5,4,3,2,1])
    expect(list).to eq(@cls[5,4,3,2,1])
  end

  it "sort_by" do
    list = @cls.new
    expect(list.sort_by{|a| a}).to eq(@cls.new)
    list.push *[4,1,3,5,2]
    expect(list.sort_by{|a| a}).to eq(@cls[1,2,3,4,5])
    expect(list.sort_by{|a| -a}).to eq(@cls[5,4,3,2,1])
    expect(list).to eq(@cls[4,1,3,5,2])
  end

  it "sort_by!" do
    list = @cls.new
    expect(list.sort_by!{|a| a}).to eq(@cls.new)
    list.push *[4,1,3,5,2]
    expect(list.sort_by!{|a| a}).to eq(@cls[1,2,3,4,5])
    expect(list.sort_by!{|a| -a}).to eq(@cls[5,4,3,2,1])
    expect(list).to eq(@cls[5,4,3,2,1])
  end

  it "collect" do
    list = @cls.new
    expect(list.collect{|a| a}).to eq(@cls.new)
    list.push *[4,1,3,5,2]
    expect(list.collect{|a| a * a}).to eq(@cls[16,1,9,25,4])
    expect(list.map.each{|a| -a}).to eq(@cls[-4,-1,-3,-5,-2])
    expect(list).to eq(@cls[4,1,3,5,2])
  end

  it "collect!" do
    list = @cls.new
    expect(list.collect!{|a| a}).to eq(@cls.new)
    list.push *[4,1,3,5,2]
    expect(list.collect!{|a| a * a}).to eq(@cls[16,1,9,25,4])
    expect(list.map!.each{|a| -a}).to eq(@cls[-16,-1,-9,-25,-4])
  end

  it "select" do
    list = @cls.new
    expect(list.select{|i| i}).to eq(@cls.new)
    list.push *[4,1,3,5,2]
    expect(list.select{|i| i.even?}).to eq(@cls[4,2])
    expect(list.select.each{|i| i.odd?}).to eq(@cls[1,3,5])
    expect(list).to eq(@cls[4,1,3,5,2])
  end

  it "select!" do
    list = @cls.new
    expect(list.select!{|i| i}).to eq(nil)
    list.push *[4,1,3,5,2]
    expect(list.select!{|i| i.even?}).to eq(@cls[4,2])
    expect(list.select!.each{|i| i % 4 == 0}).to eq(@cls[4])
    expect(list.select!.each{|i| i % 4 == 0}).to eq(nil)
    expect(list).to eq(@cls[4])
  end

  it "keep_if" do
    list = @cls.new
    expect(list.keep_if{|i| i}).to eq(list)
    list.push *[4,1,3,5,2]
    expect(list.keep_if{|i| i.even?}).to eq(@cls[4,2])
    expect(list.keep_if.each{|i| i % 4 == 0}).to eq(@cls[4])
    expect(list.keep_if.each{|i| i % 4 == 0}).to eq(list)
    expect(list).to eq(@cls[4])
  end

  it "values_at" do
    expect(@cls.new.values_at(1)).to eq(@cls[nil])
    list = @cls[4,1,3,5,2]
    expect(list.values_at).to eq(@cls.new)
    expect(list.values_at 1).to eq(@cls[1])
    expect(list.values_at 1,3,10).to eq(@cls[1,5,nil])
    expect(list.values_at -1,-2).to eq(@cls[2,5])
    expect(list.values_at 1..5).to eq(@cls[1,3,5,2,nil])
    expect(list.values_at -2..-1).to eq(@cls[5,2])
    expect(list.values_at -1..-2).to eq(@cls.new)
    expect(list.values_at 0,1..5).to eq(@cls[4,1,3,5,2,nil])
    expect(list.values_at 10..12).to eq(@cls[nil,nil,nil])
    expect{list.values_at "a"}.to raise_error(TypeError)
  end

  it "delete" do
    expect(@cls.new.delete(nil)).to eq(nil)
    list = @cls[4,1,3,2,5,5]
    expect(list.delete(1)).to eq(1)
    expect(list).to eq(@cls[4,3,2,5,5])
    expect(list.delete(5){"not found"}).to eq(5)
    expect(list).to eq(@cls[4,3,2])
    expect(list.delete(4)).to eq(4)
    expect(list.delete(1){"not found"}).to eq("not found")

    list = @cls.new(1024,0)
    expect(list.delete(0)).to eq(0)
    expect(list).to eq(@cls.new)
  end

  it "delete_at" do
    list = @cls[1,2,3]
    expect(list.delete_at(1)).to eq(2)
    expect(list).to eq(@cls[1,3])
    expect(list.delete_at(-1)).to eq(3)
    expect(list).to eq(@cls[1])
    expect(list.delete_at(0)).to eq(1)
    expect(list).to eq(@cls[])
    expect(list.delete_at(0)).to eq(nil)
    expect{list.delete_at("a")}.to raise_error(TypeError)
  end

  it "delete_if" do
    list = @cls[1,2,3,4,5]
    expect(list.delete_if{|i| 3 < i}).to eq(@cls[1,2,3])
    expect(list.delete_if.each{|i| i.even?}).to eq(@cls[1,3])
    expect(list.delete_if{|i| 0 < i}).to eq(@cls[])
    expect(list.delete_if{|i| 0 < i}).to eq(@cls[])
    expect(list).to eq(@cls[])
  end

  it "reject" do
    list = @cls[1,2,3,4,5]
    expect(list.reject{|i| 3 < i}).to eq(@cls[1,2,3])
    expect(list.reject.each{|i| i.even?}).to eq(@cls[1,3,5])
    expect(list.reject{|i| 0 < i}).to eq(@cls[])
    expect(list).to eq(@cls[1,2,3,4,5])
  end

  it "reject!" do
    list = @cls[1,2,3,4,5]
    expect(list.reject!{|i| i % 10 == 0}).to eq(nil)
    expect(list.reject!{|i| 3 < i}).to eq(@cls[1,2,3])
    expect(list.reject!.each{|i| i.even?}).to eq(@cls[1,3])
    expect(list.reject!{|i| 0 < i}).to eq(@cls[])
    expect(list.reject!{|i| 0 < i}).to eq(nil)
    expect(list).to eq(@cls[])
  end

  it "zip" do
    a = @cls[4,5,6]
    b = @cls[7,8,9]
    ary = []
    expect(a.zip(b)).to eq(@cls[[4,7],[5,8],[6,9]])
    expect(@cls[1,2,3].zip(a,b)).to eq(@cls[[1,4,7],[2,5,8],[3,6,9]])
    expect(@cls[1,2].zip(a,b)).to eq(@cls[[1,4,7],[2,5,8]])
    expect(a.zip(@cls[1,2],@cls[8])).to eq(@cls[[4,1,8],[5,2,nil],[6,nil,nil]])
    expect(a.zip([1,2],[8])).to eq(@cls[[4,1,8],[5,2,nil],[6,nil,nil]])
    expect(a.zip([1,2],[8]){|i| ary << i}).to eq(nil)
    expect(ary).to eq([[4,1,8],[5,2,nil],[6,nil,nil]])
    expect{a.zip("a")}.to raise_error  # <= 1.9.3 NoMethodError, >= 2.0.0 TypeError
    
    obj = Object.new
    def obj.to_a; [1,2]; end
    expect{@cls[*%w{a b}].zip(obj)}.to raise_error # <= 1.9.3 NoNameError, >= 2.0.0 TypeError

    def obj.each; [3,4].each{|e| yield e}; end
    expect(@cls[*%w{a b}].zip(obj)).to eq(@cls[['a',3],['b',4]])

    def obj.to_ary; [5,6]; end
    expect(@cls[*%w{a b}].zip(obj)).to eq(@cls[['a',5],['b',6]])

    r = 1..1
    def r.respond_to?(*); super; end
    expect(@cls[42].zip(r)).to eq(@cls[[42,1]])
  end

  it "transpose" do
    expect(@cls[].transpose).to eq(@cls[])
    expect(@cls[[1,2],@cls[3,4],@cls[5,6]].transpose).to eq(@cls[[1,3,5],[2,4,6]])
    expect(@cls[[1,2],[3,4],[5,6]].transpose).to eq(@cls[[1,3,5],[2,4,6]])
    expect{@cls[[1],[2,3]].transpose}.to raise_error(IndexError)
    expect{@cls[1].transpose}.to raise_error(TypeError)
  end

  it "fill" do
    list = @cls["a","b","c","d"]
    expect(list.fill("x")).to eq(@cls["x","x","x","x"])
    expect(list.fill("z",2,2)).to eq(@cls["x","x","z","z"])
    expect(list.fill("y",0..1)).to eq(@cls["y","y","z","z"])
    expect(list.fill{"foo"}).to eq(@cls["foo","foo","foo","foo"])
    expect(list[0].object_id).to_not eq(list[1].object_id)
    expect(list.fill{|i| i*i}).to eq(@cls[0,1,4,9])
    expect(list.fill(-2){|i| i*i*i}).to eq(@cls[0,1,8,27])
    expect(list.fill("z",2)).to eq(@cls[0,1,"z","z"])
    expect{list.fill("z","a")}.to raise_error(TypeError)
  end

  it "include?" do
    list = @cls.new
    expect(list.include?(1)).to eq(false)
    list.push 1,2,3
    expect(list.include?(1)).to eq(true)
    expect(list.include?(10)).to eq(false)
  end

  it "<=>" do
    expect(@cls["a","a","c"] <=> @cls["a","b","c"]).to eq(-1)
    expect(@cls[1,2,3,4,5,6] <=> @cls[1,2]).to eq(+1)
    expect(@cls[1,2] <=> @cls[1,2]).to eq(0)
    expect(@cls[1,2] <=> @cls[1,:two]).to eq(nil)
  end

  it "slice" do
    list = @cls.new
    expect(list.slice(0)).to eq(nil)
    list.push 1,2,3,4,5
    expect(list.slice(0)).to eq(1)
    expect(list.slice(1,3)).to eq(@cls[2,3,4])
    expect(list.slice(1..3)).to eq(@cls[2,3,4])
  end

  it "slice!" do
    list = @cls.new
    expect(list.slice!(0)).to eq(nil)
    list.push 1,2,3,4,5
    expect(list.slice!(2)).to eq(3)
    expect(list.slice!(2,3)).to eq(@cls[4,5])
    expect(list.slice!(0..1)).to eq(@cls[1,2])
    list.push 1,2,3,4,5
    expect(list.slice!(-1)).to eq(5)
    expect(list.slice!(100..110)).to eq(nil)
    expect(list.slice!(100)).to eq(nil)
    expect(list.slice!(100,1)).to eq(nil)
    expect{list.slice!(0xffffffffffffffff)}.to raise_error(RangeError)
    expect{list.slice!("a")}.to raise_error(TypeError)
    expect{list.slice!(1,"a")}.to raise_error(TypeError)
    expect{list.slice!(1,2,3)}.to raise_error(ArgumentError)
    expect(list).to eq(@cls[1,2,3,4])
  end

  it "assoc" do
    expect(@cls[].assoc "foo").to eq(nil)
    list = @cls[@cls["foo",1,2],["bar",1,2],[1,"baz"]]
    expect(list.assoc "foo").to eq(@cls["foo",1,2])
    expect(list.assoc "bar").to eq(@cls["bar",1,2])
    expect(list.assoc "baz").to eq(nil)
  end

  it "rassoc" do
    expect(@cls[].rassoc "foo").to eq(nil)
    list = @cls[@cls["foo",1,2],["bar",1,2],[1,"baz"]]
    expect(list.rassoc "foo").to eq(nil)
    expect(list.rassoc 1).to eq(@cls["foo",1,2])
    expect(list.rassoc "baz").to eq(@cls[1,"baz"])
  end

  it "+" do
    expect(@cls[] + @cls[]).to eq(@cls[])
    expect(@cls[1] + @cls[]).to eq(@cls[1])
    expect(@cls[] + @cls[1]).to eq(@cls[1])
    expect(@cls[1] + @cls[1]).to eq(@cls[1,1])
    expect(@cls[1] + @subcls[1]).to eq(@cls[1,1])
    expect(@subcls[1] + @cls[1]).to eq(@subcls[1,1])
    expect(@cls[1,2,3] + @cls["a","b","c"]).to eq(@cls[1,2,3,"a","b","c"])
    expect{@cls[] + 1}.to raise_error(TypeError)
  end

  it "*" do
    expect(@cls[] * 3).to eq(@cls[])
    expect(@cls[1] * 3).to eq(@cls[1,1,1])
    expect(@cls[1,2] * 3).to eq(@cls[1,2,1,2,1,2])
    expect(@cls[1,2,3] * 0).to eq(@cls[])
    expect{@cls[1,2,3] * (-3)}.to raise_error(ArgumentError)

    expect(@cls[1,2,3] * "-").to eq("1-2-3")
    expect(@cls[1,2,3] * "").to eq("123")
  end

  it "-" do
    expect(@cls[1] - @cls[1]).to eq(@cls[])
    expect(@cls[1,2,3,4,5] - @cls[2,3,4,5]).to eq(@cls[1])
    expect(@cls[1,2,1,3,1,4,1,5] - @cls[2,3,4,5]).to eq(@cls[1,1,1,1])
    list = @cls[]
    1000.times { list << 1 }
    expect(list - @cls[2]).to eq(@cls[1] * 1000)
    expect(@cls[1,2,1] - @cls[2]).to eq(@cls[1,1])
    expect(@cls[1,2,3] - @cls[4,5,6]).to eq(@cls[1,2,3])
  end

  it "&" do
    expect(@cls[1,1,3,5] & @cls[1,2,3]).to eq(@cls[1,3])
    expect(@cls[1,1,3,5] & @cls[]).to eq(@cls[])
    expect(@cls[] & @cls[1,2,3]).to eq(@cls[])
    expect(@cls[1,2,3] & @cls[4,5,6]).to eq(@cls[])
  end

  it "|" do
    expect(@cls[ ] | @cls[ ]).to eq(@cls[ ])
    expect(@cls[ ] | @cls[1]).to eq(@cls[1])
    expect(@cls[1] | @cls[ ]).to eq(@cls[1])
    expect(@cls[1] | @cls[1]).to eq(@cls[1])

    expect(@cls[1] | @cls[2]).to eq(@cls[1,2])
    expect(@cls[1,1] | @cls[2,2]).to eq(@cls[1,2])
    expect(@cls[1,2] | @cls[1,2]).to eq(@cls[1,2])

    a = @cls[*%w{a b c}]
    b = @cls[*%w{a b c d e}]
    c = a | b
    expect(b).to eq(c)
    expect(b.eql?(c)).to eq false
  end

  it "uniq" do
    a = @cls[]
    b = a.uniq
    expect(a).to eq(@cls[])
    expect(b).to eq(@cls[])
    expect(a.eql?(b)).to eq false

    a = @cls[1]
    b = a.uniq
    expect(a).to eq(@cls[1])
    expect(b).to eq(@cls[1])
    expect(a.eql?(b)).to eq false

    a = @cls[1,1]
    b = a.uniq
    expect(a).to eq(@cls[1,1])
    expect(b).to eq(@cls[1])
    expect(a.eql?(b)).to eq false

    a = @cls[1,2]
    b = a.uniq
    expect(a).to eq(@cls[1,2])
    expect(b).to eq(@cls[1,2])
    expect(a.eql?(b)).to eq false

    a = @cls[1,2,3,2,1,2,3,4,nil]
    b = a.dup
    expect(a.uniq).to eq(@cls[1,2,3,4,nil])
    expect(a).to eq(b)

    c = @cls["a:def","a:xyz","b:abc","b:xyz","c:jkl"]
    d = c.dup
    expect(c.uniq{|s| s[/^\w+/]}).to eq(@cls["a:def","b:abc","c:jkl"])
    expect(c).to eq(d)

    a = @cls[*%w{a a}]
    b = a.uniq
    expect(a).to eq(@cls[*%w{a a}])
    expect(a.none?(&:frozen?)).to eq true 
    expect(b).to eq(@cls[*%w{a}])
    expect(b.none?(&:frozen?)).to eq true

    b = "abc"
    list = @cls[b,b.dup,b.dup]
    expect(list.uniq.size).to eq(1)
    expect(list.uniq[0]).to eq(b)

    a = @cls[]
    b = a.uniq {|v| v.even? }
    expect(a).to eq(@cls[])
    expect(b).to eq(@cls[])
    expect(a.eql?(b)).to eq false

    a = @cls[1]
    b = a.uniq {|v| v.even? }
    expect(a).to eq(@cls[1])
    expect(b).to eq(@cls[1])
    expect(a.eql?(b)).to eq false

    a = @cls[1,3]
    b = a.uniq {|v| v.even? }
    expect(a).to eq(@cls[1,3])
    expect(b).to eq(@cls[1])
    expect(a.eql?(b)).to eq false

    a = @cls[*%w{a a}]
    b = a.uniq {|v| v }
    expect(a).to eq(@cls[*%w{a a}])
    expect(a.none?(&:frozen?)).to eq true 
    expect(b).to eq(@cls[*%w{a}])
    expect(b.none?(&:frozen?)).to eq true
  end

  it "uniq!" do
    a = @cls[]
    b = a.uniq!
    expect(b).to eq(nil)

    a = @cls[1]
    b = a.uniq!
    expect(b).to eq(nil)

    a = @cls[1,1]
    b = a.uniq!
    expect(a).to eq(@cls[1])
    expect(b).to eq(@cls[1])
    expect(b).to eq(a)

    a = @cls[1,2]
    b = a.uniq!
    expect(a).to eq(@cls[1,2])
    expect(b).to eq(nil)

    a = @cls[1,2,3,2,1,2,3,4,nil]
    expect(a.uniq!).to eq(@cls[1,2,3,4,nil])
    expect(a).to eq(@cls[1,2,3,4,nil])

    c = @cls["a:def","a:xyz","b:abc","b:xyz","c:jkl"]
    d = c.dup
    expect(c.uniq!{|s| s[/^\w+/]}).to eq(@cls["a:def","b:abc","c:jkl"])
    expect(c).to eq(@cls["a:def","b:abc","c:jkl"])

    c = @cls["a:def","b:abc","c:jkl"]
    expect(c.uniq!{|s| s[/^\w+/]}).to eq(nil)
    expect(c).to eq(@cls["a:def","b:abc","c:jkl"])

    expect(@cls[1,2,3].uniq!).to eq(nil)

    f = a.dup.freeze
    expect{a.uniq!(1)}.to raise_error(ArgumentError)
    expect{f.uniq!(1)}.to raise_error(ArgumentError)
    expect{f.uniq!}.to raise_error(RuntimeError)

    expect{
      a = @cls[ {c: "b"}, {c: "r"}, {c: "w"}, {c: "g"}, {c: "g"} ]
      a.sort_by!{|e| e[:c]}
      a.uniq!   {|e| e[:c]}
    }.to_not raise_error

    a = @cls[]
    b = a.uniq! {|v| v.even? }
    expect(b).to eq(nil)

    a = @cls[1]
    b = a.uniq! {|v| v.even? }
    expect(b).to eq(nil)

    a = @cls[1,3]
    b = a.uniq! {|v| v.even? }
    expect(a).to eq(@cls[1])
    expect(b).to eq(@cls[1])
    expect(a.eql?(b)).to eq true

    a = @cls[*%w{a a}]
    b = a.uniq! {|v| v }
    expect(b).to eq(@cls[*%w{a}])
    expect(a.eql?(b)).to eq true
    expect(b.none?(&:frozen?)).to eq true

    list = @cls[1,2]
    orig = list.dup
    expect{list.uniq! {|v| list.freeze; 1}}.to raise_error(RuntimeError)
    expect(list).to eq(orig)
  end

  it "compact" do
    expect(@cls[1,nil,nil,2,3,nil,4].compact).to eq(@cls[1,2,3,4])
    expect(@subcls[1,nil,nil,2,3,nil,4].compact).to eq(@subcls[1,2,3,4])
    expect(@cls[nil,1,nil,2,3,nil,4].compact).to eq(@cls[1,2,3,4])
    expect(@cls[1,nil,nil,2,3,nil,4,nil].compact).to eq(@cls[1,2,3,4])
    expect(@cls[1,2,3,4].compact).to eq(@cls[1,2,3,4])
  end

  it "compact!" do
    a = @cls[1,nil,nil,2,3,nil,4]
    expect(a.compact!).to eq(@cls[1,2,3,4])
    expect(a).to eq(@cls[1,2,3,4])

    a = @cls[nil,1,nil,2,3,nil,4]
    expect(a.compact!).to eq(@cls[1,2,3,4])
    expect(a).to eq(@cls[1,2,3,4])

    a = @cls[1,nil,nil,2,3,nil,4,nil]
    expect(a.compact!).to eq(@cls[1,2,3,4])
    expect(a).to eq(@cls[1,2,3,4])

    a = @cls[1,2,3,4]
    expect(a.compact!).to eq(nil)
    expect(a).to eq(@cls[1,2,3,4])
  end

  it "flatten" do
    a1 = @cls[1,2,3]
    a2 = @cls[5,6]
    a3 = @cls[4,a2]
    a4 = @cls[a1,a3]
    expect(a4.flatten).to eq(@cls[1,2,3,4,5,6])
    expect(a4.flatten(1)).to eq(@cls[1,2,3,4,@cls[5,6]])
    expect(a4).to eq(@cls[a1,a3])

    a5 = @cls[a1,@cls[],a3]
    expect(a5.flatten).to eq(@cls[1,2,3,4,5,6])
    expect(@cls[].flatten).to eq(@cls[])
    expect(@cls[@cls[@cls[@cls[],@cls[]],@cls[@cls[]],@cls[]],@cls[@cls[@cls[]]]].flatten).to eq(@cls[])

    expect{@cls[@cls[]].flatten("")}.to raise_error(TypeError)

    a6 = @cls[@cls[1,2],3]
    a6.taint
    a7 = a6.flatten
    expect(a7.tainted?).to eq true

    a8 = @cls[@cls[1,2],3]
    a9 = a8.flatten(0)
    expect(a9).to eq(a8)
    expect(a9.eql?(a8)).to eq false

    f = [].freeze
    expect{f.flatten!(1,2)}.to raise_error(ArgumentError)
    expect{f.flatten!}.to raise_error(RuntimeError)
    expect{f.flatten!(:foo)}.to raise_error(RuntimeError)
  end

  it "flatten!" do
    a1 = @cls[1,2,3]
    a2 = @cls[5,6]
    a3 = @cls[4,a2]
    a4 = @cls[a1,a3]
    expect(a4.flatten!).to eq(@cls[1,2,3,4,5,6])
    expect(a4).to eq(@cls[1,2,3,4,5,6])

    a5 = @cls[a1,@cls[],a3]
    expect(a5.flatten!).to eq(@cls[1,2,3,4,5,6])
    expect(a5.flatten!(0)).to eq(nil)
    expect(a5).to eq(@cls[1,2,3,4,5,6])

    expect(@cls[].flatten!).to eq(nil)
    expect(@cls[@cls[@cls[@cls[],@cls[]],@cls[@cls[]],@cls[]],@cls[@cls[@cls[]]]].flatten!).to eq(@cls[])

    expect(@cls[].flatten!(0)).to eq(nil)
  end

  it "count" do
    a = @cls[1,2,3,1,2]
    expect(a.count).to eq(5)
    expect(a.count(1)).to eq(2)
    expect(a.count {|x| x % 2 == 1}).to eq(3)
    expect(a.count(1) {|x| x % 2 == 1}).to eq(2)
    expect{a.count(0,1)}.to raise_error(ArgumentError)
  end

  it "shuffle" do
    100.times do
      expect(@cls[2,1,0].shuffle.sort).to eq(@cls[0,1,2])
    end

    gen = Random.new(0)
    expect{@cls[1,2,3].shuffle(1, random: gen)}.to raise_error(ArgumentError)
    srand(0)
    100.times do
      expect(@cls[0,1,2].shuffle(random: gen)).to eq(@cls[0,1,2].shuffle)
    end

    # FIXME v1.9.3 and v2.0.0 nothing raise
    # expect{@cls[0,1,2].shuffle(xawqij: "a")}.to raise_error(ArgumentError)
    # expect{@cls[0,1,2].shuffle!(xawqij: "a")}.to raise_error(ArgumentError)

    gen = proc do
      10000000
    end
    class << gen
      alias rand call
    end
    expect{@cls[*0..2].shuffle(random: gen)}.to raise_error(RangeError)

    # FIXME
    # list = @cls[*(0...10000)]
    # gen = proc do
    #   list.replace(@cls[])
    #   0.5
    # end
    # class << gen
    #   alias rand call
    # end
    # expect{list.shuffle!(random: gen)}.to raise_error(RuntimeError)

    # zero = Object.new
    # def zero.to_int
    #   0
    # end
    # gen_to_int = proc do |max|
    #   zero
    # end
    # class << gen_to_int
    #   alias rand call
    # end
    # list = @cls[*(0...10000)]
    # expect(list.shuffle(random: gen_to_int)).to eq(list.rotate)
  end

  it "sample" do
    100.times do
      s = [2,1,0].sample
      expect(s).to be_a_kind_of(Fixnum)
      expect(s).to be >= 0
      expect(s).to be <= 2
      @cls[2,1,0].sample(2).each {|sample|
        expect(sample).to be_a_kind_of(Fixnum)
        expect(sample).to be >= 0
        expect(sample).to be <= 2
      }
    end

    srand(0)
    a = @cls[*1..18]
    (0..20).each do |n|
      100.times do
        b = a.sample(n)
        expect(b.size).to eq([n,18].min)
        expect((a | b).sort).to eq(a)
        expect((a & b).sort).to eq(b.sort)
      end

      h = Hash.new(0)
      1000.times do
        a.sample(n).each {|x| h[x] += 1 }
      end
      expect(h.values.max).to be <= (h.values.min * 2) if n != 0
    end

    expect{[1,2].sample(-1)}.to raise_error(ArgumentError)
  
    gen = Random.new(0)
    srand(0)
    a = @cls[*(1..18)]
    (0..20).each do |n|
      100.times do |i|
        expect(a.sample(n, random: gen)).to eq(a.sample(n))
      end
    end

    # FIXME v1.9.3 and v2.0.0 nothing raise
    # expect{@cls[0,1,2].sample(xawqij: "a")}.to raise_error(ArgumentError)
  end

  it "cycle" do
    a = @cls[]
    @cls[0,1,2].cycle do |i|
      a << i
      break if a.size == 10
    end
    expect(a).to eq(@cls[0,1,2,0,1,2,0,1,2,0])

    a = @cls[0,1,2]
    expect(a.cycle {a.clear}).to eq(nil)

    a = @cls[]
    @cls[0,1,2].cycle(3) {|i| a << i}
    expect(a).to eq(@cls[0,1,2,0,1,2,0,1,2])

    expect(@cls[0,1,2].cycle(3).to_a.size).to eq(9)

    expect(@cls[0,1,2].cycle(-1){}).to eq(nil)
    expect{@cls[0,1,2].cycle("a"){}}.to raise_error(TypeError)
  end

  it "permutation" do
    a = @cls[1,2,3]
    expect(a.permutation(0).to_list).to eq(@cls[[]])
    expect(a.permutation(1).to_list.sort).to eq(@cls[[1],[2],[3]])
    expect(a.permutation(2).to_list.sort).to eq(@cls[[1,2],[1,3],[2,1],[2,3],[3,1],[3,2]])
  end

  it "compatible" do
    expect(@cls[1,2,3,4].combination(0).to_list).to eq(@cls[[]])
    expect(@cls[1,2,3,4].combination(1).to_list).to eq(@cls[[1],[2],[3],[4]])
    expect(@cls[1,2,3,4].combination(2).to_list).to eq(@cls[[1,2],[1,3],[1,4],[2,3],[2,4],[3,4]])
  end

  it "repeated_combination" do
    a = @cls[1,2,3]
    expect(a.repeated_combination(0).to_list).to eq(@cls[[]])
    expect(a.repeated_combination(1).to_list.sort).to eq(@cls[[1],[2],[3]])
    expect(a.repeated_combination(2).to_list.sort).to eq(@cls[[1,1],[1,2],[1,3],[2,2],[2,3],[3,3]])
  end

  it "repeated_permutation" do
    a = @cls[1,2]
    expect(a.repeated_permutation(0).to_list).to eq(@cls[[]])
    expect(a.repeated_permutation(1).to_list.sort).to eq(@cls[[1],[2]])
    expect(a.repeated_permutation(2).to_list.sort).to eq(@cls[[1,1],[1,2],[2,1],[2,2]])
  end

  it "product" do
    expect(@cls[1,2,3].product([4,5])).to eq(@cls[[1,4],[1,5],[2,4],[2,5],[3,4],[3,5]])
    expect(@cls[1,2].product([1,2])).to eq(@cls[[1,1],[1,2],[2,1],[2,2]])
    expect(@subcls[1,2].product([1,2])).to eq(@subcls[[1,1],[1,2],[2,1],[2,2]])
  end

  it "take" do
    expect(@cls[1,2,3,4,5,0].take(3)).to eq(@cls[1,2,3])
    expect(@subcls[1,2,3,4,5,0].take(3)).to eq(@subcls[1,2,3])
    expect{@cls[1,2].take(-1)}.to raise_error(ArgumentError)
    expect(@cls[1,2].take(1000000000)).to eq(@cls[1,2])
  end

  it "take_while" do
    expect(@cls[1,2,3,4,5,0].take_while {|i| i < 3}).to eq(@cls[1,2])
  end

  it "drop" do
    expect(@cls[1,2,3,4,5,0].drop(3)).to eq(@cls[4,5,0])
    expect{@cls[1,2].drop(-1)}.to raise_error(ArgumentError)
    expect(@cls[1,2].drop(1000000000)).to eq(@cls[])
  end

  it "drop_while" do
    expect(@cls[1,2,3,4,5,0].drop_while {|i| i < 3 }).to eq(@cls[3,4,5,0])
    expect(@subcls[1,2,3,4,5,0].drop_while {|i| i < 3 }).to eq(@subcls[3,4,5,0])
  end

  it "bsearch" do
    expect{@cls[1, 2, 42, 100, 666].bsearch{ "not ok" }}.to raise_error(TypeError)
    expect(@cls[1, 2, 42, 100, 666].bsearch{false}).to eq(@cls[1, 2, 42, 100, 666].bsearch{})
    enum = @cls[1, 2, 42, 100, 666].bsearch
    if enum.respond_to? :size # v1.9.3 not respond to :size
      expect(enum.size).to eq nil
    end
    expect(enum.each{|x| x >= 33}).to eq(42)

    a = @cls[0,4,7,10,12]
    expect(a.bsearch {|x| 4 - x / 2 }).to eq nil
  end

  it "ring" do
    len = 0
    result = []
    list = @cls[1,2,3]
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
    list = @cls[1,2,3]
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
    list = @cls[1,2,3]
    expect(list.ring?).to eq(false)
    expect(list.ring.ring?).to eq(true)
    expect(list.ring!.ring?).to eq(true)
  end

  it "pack" do
    expect(@cls[76,105,115,116].pack("C*")).to eq("List")
  end
end
