%module swdb

%begin %{
#define SWIG_PYTHON_2_UNICODE
%}

%include <std_map.i>
%include <std_pair.i>
%include <std_shared_ptr.i>
%include <std_string.i>
%include <std_vector.i>
// %include <stdint.i>
%include <typemaps.i>


%rename ("__hash__", fullname=1) "TransactionItem::getHash";


%exception {
    try {
        $action
    }
    catch (const std::exception & e)
    {
       SWIG_exception(SWIG_RuntimeError, (std::string("C++ std::exception: ") + e.what()).c_str());
    }
    catch (...)
    {
       SWIG_exception(SWIG_UnknownError, "C++ anonymous exception");
    }
}


%shared_ptr(SQLite3)
typedef std::shared_ptr< SQLite3 > SQLite3Ptr;

%shared_ptr(Item)
typedef std::shared_ptr< Item > ItemPtr;

%shared_ptr(RPMItem)
typedef std::shared_ptr< RPMItem > RPMItemPtr;

%shared_ptr(CompsGroupItem)
typedef std::shared_ptr< CompsGroupItem > CompsGroupItemPtr;

%shared_ptr(CompsGroupPackage)
typedef std::shared_ptr< CompsGroupPackage > CompsGroupPackagePtr;

%shared_ptr(CompsEnvironmentItem)
typedef std::shared_ptr< CompsEnvironmentItem > CompsEnvironmentItemPtr;

%shared_ptr(CompsEnvironmentGroup)
typedef std::shared_ptr< CompsEnvironmentGroup > CompsEnvironmentGroupPtr;

%shared_ptr(libdnf::Transaction)
typedef std::shared_ptr< libdnf::Transaction > TransactionPtr;

%shared_ptr(TransactionItem)
typedef std::shared_ptr< TransactionItem > TransactionItemPtr;

%shared_ptr(MergedTransaction)
typedef std::shared_ptr< MergedTransaction > MergedTransactionPtr;

%shared_ptr(TransactionItemBase)
typedef std::shared_ptr< TransactionItemBase > TransactionItemBasePtr;

// FIXME this is a dirty workaround - file a patch to the swig upstream
typedef long int int64_t;
typedef unsigned int uint32_t;
typedef int int32_t;

%{
    // make SWIG wrap following headers
    #include "libdnf/swdb/item.hpp"
    #include "libdnf/swdb/item_comps_environment.hpp"
    #include "libdnf/swdb/item_comps_group.hpp"
    #include "libdnf/swdb/item_rpm.hpp"
    #include "libdnf/swdb/swdb.hpp"
    #include "libdnf/swdb/swdb_types.hpp"
    #include "libdnf/swdb/transaction.hpp"
    #include "libdnf/swdb/transactionitem.hpp"
    #include "libdnf/swdb/mergedtransaction.hpp"
%}


%template() std::vector<std::shared_ptr<libdnf::Transaction> >;
%template() std::vector<std::shared_ptr<TransactionItem> >;
%template() std::vector<std::shared_ptr<TransactionItemBase> >;
%template() std::vector<std::shared_ptr<CompsGroupPackage> >;
%template() std::vector<std::shared_ptr<CompsEnvironmentGroup> >;

%template() std::vector<uint32_t>;
%template() std::vector<int64_t>;
%template() std::vector<bool>;

%template() std::vector< std::string >;
%template() std::pair<int,std::string>;
%template() std::map<std::string,int>;
%template() std::map<std::string,std::string>;
%template() std::vector<std::pair<int,std::string> >;

// make SWIG look into following headers
%include "libdnf/swdb/item.hpp"
%include "libdnf/swdb/item_comps_environment.hpp"
%include "libdnf/swdb/item_comps_group.hpp"
%include "libdnf/swdb/item_rpm.hpp"
%include "libdnf/swdb/swdb.hpp"
%include "libdnf/swdb/swdb_types.hpp"
%include "libdnf/swdb/transaction.hpp"
%include "libdnf/swdb/transactionitem.hpp"
%include "libdnf/swdb/mergedtransaction.hpp"
