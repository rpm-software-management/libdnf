require 'test/unit/assertions'
include Test::Unit::Assertions

require 'libdnf/utils'

assert_equal Utils.random_int32(42,42), 42, 'Utils.random_int32(42,42) should return 42'
