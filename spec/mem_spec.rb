require 'spec_helper'

describe List do
  it "gc safe" do
    list = (0...3000).to_list
    GC.start
    expect(list.length).to eq(3000)
  end
end
