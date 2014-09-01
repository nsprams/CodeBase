#ARRAYS

myarray = %w{one two three four}
myarray.each {|element| print "[" + element + "]... "}

puts
#Alternative to 

myarray.each do |element|
  print "[" + element + "].."
end


#PROC

myproc = Proc.new {|animal| puts "I hate #{animal}!"}
myproc.call("women")


def make_show_name(show)
  Proc.new {|host| show + " with " + host}
end

show1 = make_show_name ("Practical Cannibalism")
show1.call("H Annabellector")



