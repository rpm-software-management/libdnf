use strict;
use warnings;
use Test::More tests => 7;

use libdnf::sack;

my $osack = libdnf::sack::ObjectSack->new();
my $data = $osack->get_data();

my $o1 = libdnf::sack::Object->new();
$o1->swig_string_set('Test o1');
$o1->swig_int64_set(64);
$data->push($o1);

my $o2 = libdnf::sack::Object->new();
$o2->swig_string_set('Test o2');
$o2->swig_int64_set(264);
$data->push($o2);

my $query = $osack->new_query();
is($query->size(), 2, 'Number of items in the newly created query.');

my $removed = $query->filter($libdnf::sack::ObjectQuery_get_string_cb, $libdnf::sack::QueryCmp_EQ, 'Test o2');
is($removed, 1, 'Number of removed items from query using filter.');
is($query->size(), 1, 'Number of items in the query after using filter');

my $removed = $query->filter($libdnf::sack::ObjectQuery_get_int64_cb, $libdnf::sack::QueryCmp_EQ, 264);
is($removed, 0, 'Number of removed items from query using filter.');
is($query->size(), 1, 'Number of items in the query after using filter');

my $removed = $query->filter($libdnf::sack::ObjectQuery_get_int64_cb, $libdnf::sack::QueryCmp_EQ, 64);
is($removed, 1, 'Number of removed items from query using filter.');
is($query->size(), 0, 'Number of items in the query after using filter');
