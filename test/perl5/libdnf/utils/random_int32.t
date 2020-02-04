use strict;
use warnings;
use Test::More tests => 1;
use libdnf::utils;
is(libdnf::utils::random_int32(42, 42), 42, 'random_int32(42, 42)');
