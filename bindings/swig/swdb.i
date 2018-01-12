%module swdb


%include <std_string.i>
%include <stdint.i>


%{
    // make SWIG wrap following headers
    #include "libdnf/swdb/item_comps_environment.hpp"
    #include "libdnf/swdb/item_comps_group.hpp"
    #include "libdnf/swdb/item.hpp"
    #include "libdnf/swdb/item_rpm.hpp"
    #include "libdnf/swdb/repo.hpp"
    #include "libdnf/swdb/swdb.hpp"
    #include "libdnf/swdb/transaction.hpp"
    #include "libdnf/swdb/transactionitem.hpp"
%}


// make SWIG look into following headers
%include "libdnf/swdb/item_comps_environment.hpp"
%include "libdnf/swdb/item_comps_group.hpp"
%include "libdnf/swdb/item_rpm.hpp"
%include "libdnf/swdb/repo.hpp"
%include "libdnf/swdb/swdb.hpp"
%include "libdnf/swdb/transaction.hpp"
%include "libdnf/swdb/transactionitem.hpp"
