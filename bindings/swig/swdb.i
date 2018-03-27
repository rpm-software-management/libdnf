%module swdb

%begin %{
#define SWIG_PYTHON_2_UNICODE
%}

%include <std_map.i>
%include <std_pair.i>
%include <std_shared_ptr.i>
%include <std_string.i>
%include <std_vector.i>
%include <stdint.i>
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

%{
    // make SWIG wrap following headers
    #include "libdnf/swdb/Item.hpp"
    #include "libdnf/swdb/CompsEnvironmentItem.hpp"
    #include "libdnf/swdb/CompsGroupItem.hpp"
    #include "libdnf/swdb/RPMItem.hpp"
    #include "libdnf/swdb/Swdb.hpp"
    #include "libdnf/swdb/SwdbTypes.hpp"
    #include "libdnf/swdb/Transaction.hpp"
    #include "libdnf/swdb/TransactionItem.hpp"
    #include "libdnf/swdb/MergedTransaction.hpp"
    #include "libdnf/swdb/Transformer.hpp"
%}


// This include has to be before %template definitions. SWIG is fragile :(
%include "libdnf/swdb/SwdbTypes.hpp"


%template() std::vector<std::shared_ptr<libdnf::Transaction> >;
%template() std::vector<std::shared_ptr<TransactionItem> >;
%template() std::vector<std::shared_ptr<TransactionItemBase> >;
%template() std::vector<std::shared_ptr<CompsGroupPackage> >;
%template() std::vector<std::shared_ptr<CompsEnvironmentGroup> >;
%template(TransactionStateVector) std::vector<TransactionState>;

%template() std::vector<uint32_t>;
%template() std::vector<int64_t>;
%template() std::vector<bool>;

%template() std::vector< std::string >;
%template() std::pair<int,std::string>;
%template() std::map<std::string,int>;
%template() std::map<std::string,std::string>;
%template() std::vector<std::pair<int,std::string> >;


// make SWIG look into following headers
%include "libdnf/swdb/Item.hpp"
%include "libdnf/swdb/CompsEnvironmentItem.hpp"
%include "libdnf/swdb/CompsGroupItem.hpp"
%include "libdnf/swdb/RPMItem.hpp"
%include "libdnf/swdb/Swdb.hpp"
%include "libdnf/swdb/Transaction.hpp"
%include "libdnf/swdb/TransactionItem.hpp"
%include "libdnf/swdb/MergedTransaction.hpp"
%include "libdnf/swdb/Transformer.hpp"
