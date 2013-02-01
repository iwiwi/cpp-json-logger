#!/usr/bin/env ruby

require 'fileutils'
require 'rubygems'
require 'json'

def new_name_json(j)
  # Edit here!!
  return j["indexing"]["algorithm"] + "_" +
    j["setting"]["graph_file"].to_s.split("/")[-1]
end

def new_name_file(f)
  return new_name_json(JSON.parse(open(f).read))
end

def new_name_files(fs)
  fs = fs.map{ |f| new_name_file(f) }

  cnt = (cnt2 = {}).dup

  fs.each{ |f| cnt[f] = cnt2[f] = 0 }
  fs.each{ |f| cnt[f] += 1 }

  fs.each_with_index do |f, i|
    if cnt[f] > 1
      cnt2[f] += 1
      fs[i] += "_" + cnt2[f].to_s
    end
  end

  return fs.map{ |f| f + ".json" }
end

if __FILE__ == $0
  org_name = Dir.glob("*")
  new_name = new_name_files(org_name)

  pairs = [org_name, new_name].transpose
  puts pairs.map { |a, b| a.to_s + " -> " + b }.join("\n")

  loop do
    puts("--\nOK? (y)")
    break if gets.strip == "y"
  end

  pairs.each do |a, b|
    FileUtils.mv(a, b) if a != b
  end
end
