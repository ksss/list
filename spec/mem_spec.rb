require 'spec_helper'

describe List do
  it "gc safe" do
    a = (0..1000).to_a
    list = List.new a
    GC.start
    expect(list.to_a).to eq(a)
  end
end
