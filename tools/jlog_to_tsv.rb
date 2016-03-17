#!/usr/bin/env ruby

require 'optparse'
require 'rubygems'
require 'json'

$html_mode = false
$pretty_mode = true

def rec(data, path)  # path in array
  while data.class != Array && path.size > 0
    cmd = path[0]
    path.delete_at(0)
    return [] if !data.key?(cmd)
    data = data[cmd]
  end

  if data.class != Array
    return [data]
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

def is_num(s)
  return /^[-+]?(?:[0-9]+(\.[0-9]*)?|(\.[0-9]+))([eE][-+]?[0-9]+)?$/ =~ s
end

def prettify(s)
  if /^[0-9]+$/ =~ s
    return s.gsub(/(\d)(?=(?:\d\d\d)+(?!\d))/, '\1,')
  elsif /^[-+]?(?:[0-9]+(\.[0-9]*)?|(\.[0-9]+))([eE][-+]?[0-9]+)?$/ =~ s
    return sprintf("%.6f", s.to_f)

    # float
    x = s.to_f
    a = (Math.log10(x.abs)).floor
    b = x / (10 ** a)
    if a == 0
      return sprintf("%.2f", x)
    else
      if $html_mode
        return sprintf("%.2f&times;10<sup>%d</sup>", b, a)
      else
        return sprintf("%.2f * 10^%d", b, a)
      end
    end
  else
    return s
  end
end

HTML_HEADER = <<EOS
<html><head>
<link rel="stylesheet" href="http://ajax.googleapis.com/ajax/libs/jqueryui/1.9.1/themes/base/jquery-ui.css" />
<script src="http://ajax.googleapis.com/ajax/libs/jquery/1.8.3/jquery.min.js"></script>
<script src="http://ajax.googleapis.com/ajax/libs/jqueryui/1.9.1/jquery-ui.min.js"></script>
<script type="text/javascript" src="http://paramquery.com/Content/js/pqgrid.min.js"></script>
<link rel="stylesheet" href="http://paramquery.com/Content/css/pqgrid.min.css" />
<script>
function is_numeric(mixed_var) {
  return (typeof(mixed_var) === 'number' || typeof(mixed_var) === 'string') && mixed_var !== '' && !isNaN(mixed_var);
}
$(function() {
     function changeToTable(that) {
            var tbl = $("table.fortune500");
            tbl.css("display", "");
            $("#grid_table").pqGrid("destroy");
            $(that).val("Table -> Grid");
        }
        function changeToGrid(that) {
            var tbl = $("table.fortune500");
            var obj = $.paramquery.tableToArray(tbl);
            var newObj = { width: "100%", height: 600, resizable: true };
            newObj.dataModel = { data: obj.data, rPP: 20, paging: "local" };
            newObj.colModel = obj.colModel;
            for (var i = 0; i < newObj.colModel.length; ++i) {
                 newObj.colModel[i].dataType = function(a, b) {
var at = a.replace(/,/g, "");
var bt = b.replace(/,/g, "");
if (is_numeric(at) && is_numeric(bt))
  return parseFloat(at) - parseFloat(bt);
else
  return a < b ? -1 : a == b ? 0 : 1;
                 }
            }
            $("#grid_table").pqGrid(newObj);
            $("#grid_table").pqGrid("option", "editable", false);
            $(that).val("Change Grid back to Table");
            tbl.css("display", "none");
            $(that).val("Grid -> Table");
        }

        $("#pq-grid-table-btn").button().click(function () {
            if ($("#grid_table").hasClass('pq-grid')) {
                changeToTable(this);
            }
            else{
                changeToGrid(this);
            }
        });
        changeToGrid($("#pq-grid-table-btn"));
    });
</script>
</head>
<body style="margin: 0">
<div id="grid_table"></div>
<table border=1 class="fortune500">
EOS

HTML_FOOTER = <<EOS
</table>
<br><input type="button" id="pq-grid-table-btn" label="Table -> Grid">
</body></html>
EOS

if __FILE__ == $0
  opts = OptionParser.new
  opts.on("--html"){$html_mode = true}
  opts.parse!(ARGV)

  data = JSON.parse($stdin.read)
  table = ARGV.map { |path| rec(data, path.split('.')) }

  len = table.map { |c| c.length }.max

  # p table
  if $html_mode
    puts(HTML_HEADER)
    puts("<tr><th>" + ARGV.join("</th><th>").gsub(".", "/<br>").gsub("_", " ") + "</th></tr>")
    len.times do |i|
      puts("<tr>")
      table.each do |c|
        x = i < c.length ? c[i] : ""
        printf("<td align=\"%s\">%s</td>",
               is_num(x) ? "right" : "left",
               prettify(x))
      end
      puts("</tr>")
    end
    puts(HTML_FOOTER)
  else
    len.times do |i|
      puts(table.map { |c| i < c.length ? c[i] : "" }.join("\t"))
    end
  end
end
