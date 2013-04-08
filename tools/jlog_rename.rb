#!/usr/bin/env ruby

require 'fileutils'
require 'rubygems'
require 'json'

def new_name_json(j)  # j
  return j["run"]["program"] + "_" + j["setting"]["graph"]
end

def new_name_file(f)
  begin
    return new_name_json(JSON.parse(open(f).read))
  rescue => e
    $stderr.puts("#{f}: #{e.to_str}")
    return f
  end
end

def new_name_files(original_fs)
  fs = original_fs.map{ |f| new_name_file(f) }

  cnt = (cnt2 = {}).dup

  fs.each{ |f| cnt[f] = cnt2[f] = 0 }
  fs.each{ |f| cnt[f] += 1 }

  [fs, original_fs].transpose.each_with_index do |ff, i|
    if ff[0] == ff[1]
      # do nothing
    else
      f = ff[0]
      if cnt[f] > 1
        cnt2[f] += 1
        fs[i] += "_" + cnt2[f].to_s
      end
      fs[i] += ".json"
    end
  end

  return fs
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
