require 'spec_helper'

describe LinkedList do
  it "gc safe" do
    a = (0..3).to_a
    list = LinkedList.new a
    GC.start
    expect(list.to_a).to eq(a)
    expect(a.length).to eq(4)
  end
end
