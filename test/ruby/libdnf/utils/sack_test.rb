require 'test/unit/assertions'
include Test::Unit::Assertions

require 'libdnf/sack'

osack = Sack::ObjectSack.new()
data = osack.get_data()

o1 = Sack::Object.new()
o1.string = "Test o1"
o1.int64 = 64
data.push(o1)

o2 = Sack::Object.new()
o2.string = "Test o2"
o2.int64 = 264
data.push(o2)

query = osack.new_query()
assert_equal query.size, 2, 'Number of items in the newly created query'

removed = query.filter(Sack::ObjectQuery_get_string_cb, Sack::QueryCmp_EQ, 'Test o2')
assert_equal removed, 1, 'Number of removed items from query using filter'
assert_equal query.size, 1, 'Number of items in the query after using filter'

removed = query.filter(Sack::ObjectQuery_get_int64_cb, Sack::QueryCmp_EQ, 264)
assert_equal removed, 0, 'Number of removed items from query using filter'
assert_equal query.size, 1, 'Number of items in the query after using filter'

removed = query.filter(Sack::ObjectQuery_get_int64_cb, Sack::QueryCmp_EQ, 64)
assert_equal removed, 1, 'Number of removed items from query using filter'
assert_equal query.size, 0, 'Number of items in the query after using filter'
