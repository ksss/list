require 'spec_helper'

describe List do
  it "class" do
    expect(List).to be_include(Enumerable)
  end

  it "initialize" do
    expect(List.new).to be_a_kind_of(List)
    expect(List.new([])).to be_a_kind_of(List)
    expect(List.new([1])).to be_a_kind_of(List)
  end

  it "to_a" do
    list = List.new
    expect(list.to_a).to eq([])
    a = (0..100).to_a
    list = List.new(a)
    expect(list.to_a).to eq(a)
  end
end
