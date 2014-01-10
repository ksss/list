require 'spec_helper'

describe List do
  it "gc safe" do
    list = List.new([1,2,3])
    GC.start
    expect(list.to_a).to eq([1,2,3])
  end
end
