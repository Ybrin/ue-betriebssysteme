fname = "sample2.txt"
somefile = File.open(fname, "a")
for i in 0..2147483649
somefile.write "b"
end
somefile.close
