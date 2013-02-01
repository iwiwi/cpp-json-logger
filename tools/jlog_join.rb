#!/usr/bin/env ruby

if __FILE__ == $0
  puts "["
  puts ARGV.map{ |f| open(f).read }.join(",\n")
  puts "]"
end
