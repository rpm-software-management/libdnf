%module swdb


%include <std_map.i>
%include <std_pair.i>
%include <std_shared_ptr.i>
%include <std_string.i>
%include <std_vector.i>
%include <stdint.i>
%include <typemaps.i>


%shared_ptr(SQLite3)

%shared_ptr(Item)
%shared_ptr(RPMItem)

%shared_ptr(CompsGroupItem)
%shared_ptr(CompsGroupPackage)

%shared_ptr(CompsEnvironmentItem)
%shared_ptr(CompsEnvironmentGroup)

%shared_ptr(Transaction)
%shared_ptr(TransactionItem)
%shared_ptr(Repo)


%{
    // make SWIG wrap following headers
    #include "libdnf/swdb/item.hpp"
    #include "libdnf/swdb/item_comps_environment.hpp"
    #include "libdnf/swdb/item_comps_group.hpp"
    #include "libdnf/swdb/item_rpm.hpp"
    #include "libdnf/swdb/repo.hpp"
    #include "libdnf/swdb/swdb.hpp"
    #include "libdnf/swdb/swdb_types.hpp"
    #include "libdnf/swdb/transaction.hpp"
    #include "libdnf/swdb/transactionitem.hpp"
%}


namespace std {
    %template() std::vector<shared_ptr<Transaction> >;
    %template() std::vector<shared_ptr<TransactionItem> >;

    %template() std::vector<shared_ptr<CompsGroupPackage> >;
    %template() std::vector<shared_ptr<CompsEnvironmentGroup> >;

    %template() vector< std::string >;
    %template() std::pair<int,std::string>;
    %template() std::map<std::string,int>;
    %template() std::map<std::string,std::string>;
    %template() std::vector<std::pair<int,std::string> >;
}


// make SWIG look into following headers
%include "libdnf/swdb/item.hpp"
%include "libdnf/swdb/item_comps_environment.hpp"
%include "libdnf/swdb/item_comps_group.hpp"
%include "libdnf/swdb/item_rpm.hpp"
%include "libdnf/swdb/repo.hpp"
%include "libdnf/swdb/swdb.hpp"
%include "libdnf/swdb/swdb_types.hpp"
%include "libdnf/swdb/transaction.hpp"
%include "libdnf/swdb/transactionitem.hpp"
