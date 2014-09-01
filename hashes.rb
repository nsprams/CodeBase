my_hash = Hash.new("Not existing key")
my_hash = { 'one' => 'first one',
	    'two' => 'second one',
	    'three' => 'third one', 'four' => 'fourth one' }

puts "Is this hash empty? " 
puts my_hash.empty?

puts "Printing value for non"
puts my_hash['non']


puts "printing value for one"
puts my_hash['one']

my_hash['one'] = 'test'
puts my_hash['one']

# freezing an object 
#my_crazy_object.freeze

#REGEX
#my_string =~ /\sstring\s/


#METHODS
def methodname(parameter)
	puts "testing methods" + parameter
end

methodname('Pramod')