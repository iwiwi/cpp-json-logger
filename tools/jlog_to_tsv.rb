#!/usr/bin/env ruby

require 'optparse'
require 'rubygems'
require 'json'

$html_mode = false

def rec(data, path)  # path in array
  while data.class != Array && path.size > 0
    cmd = path[0]
    path.delete_at(0)
    return [] if !data.key?(cmd)
    data = data[cmd]
  end

  res = []
  data.each do |d|
    if path.size == 0
      res.push(d)
    else
      res.concat(rec(d, path.dup))
    end
  end

  return res
end

def json_to_array(data, path)  # path in strings
  return rec(data, path.split('.'))
end

if __FILE__ == $0
  opts = OptionParser.new
  opts.on("--html"){$html_mode = true}
  opts.parse!(ARGV)

  data = JSON.parse($stdin.read)
  table = ARGV.map { |path| rec(data, path.split('.')) }

  len = table.map{ |c| c.length }.max

  # p table
  if $html_mode
    puts("<table border=1>")
    puts("<tr><th>" + ARGV.join("</th><th>") + "</th></tr>")
    len.times do |i|
      puts("<tr><td>" +
           table.map { |c| i < c.length ? c[i] : "" }.join("</td><td>") +
           "</td></tr>")
    end
    puts("</table>")
  else
    len.times do |i|
      puts(table.map { |c| i < c.length ? c[i] : "" }.join("\t"))
    end
  end
end
