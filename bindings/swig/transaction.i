%module transaction

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


%rename ("__hash__", fullname=1) "libdnf::TransactionItem::getHash";


%exception {
    try {
        $action
    } catch (const std::exception & e) {
       SWIG_exception(SWIG_RuntimeError, e.what());
    }
}


%shared_ptr(SQLite3)
typedef std::shared_ptr< SQLite3 > SQLite3Ptr;

%shared_ptr(libdnf::Item)
typedef std::shared_ptr< libdnf::Item > ItemPtr;

%shared_ptr(libdnf::RPMItem)
typedef std::shared_ptr< libdnf::RPMItem > RPMItemPtr;

%shared_ptr(libdnf::CompsGroupItem)
typedef std::shared_ptr< libdnf::CompsGroupItem > CompsGroupItemPtr;

%shared_ptr(libdnf::CompsGroupPackage)
typedef std::shared_ptr< libdnf::CompsGroupPackage > CompsGroupPackagePtr;

%shared_ptr(libdnf::CompsEnvironmentItem)
typedef std::shared_ptr< libdnf::CompsEnvironmentItem > CompsEnvironmentItemPtr;

%shared_ptr(libdnf::CompsEnvironmentGroup)
typedef std::shared_ptr< libdnf::CompsEnvironmentGroup > CompsEnvironmentGroupPtr;

%shared_ptr(libdnf::Transaction)
typedef std::shared_ptr< libdnf::Transaction > TransactionPtr;

%shared_ptr(libdnf::TransactionItem)
typedef std::shared_ptr< libdnf::TransactionItem > TransactionItemPtr;

%shared_ptr(libdnf::MergedTransaction)
typedef std::shared_ptr< libdnf::MergedTransaction > MergedTransactionPtr;

%shared_ptr(libdnf::TransactionItemBase)
typedef std::shared_ptr< libdnf::TransactionItemBase > TransactionItemBasePtr;

// XXX workaround - because SWIG...
typedef libdnf::CompsPackageType CompsPackageType;

%{
    // make SWIG wrap following headers
    #include "libdnf/transaction/Item.hpp"
    #include "libdnf/transaction/CompsEnvironmentItem.hpp"
    #include "libdnf/transaction/CompsGroupItem.hpp"
    #include "libdnf/transaction/RPMItem.hpp"
    #include "libdnf/transaction/Swdb.hpp"
    #include "libdnf/transaction/Transaction.hpp"
    #include "libdnf/transaction/TransactionItem.hpp"
    #include "libdnf/transaction/TransactionItemReason.hpp"
    #include "libdnf/transaction/MergedTransaction.hpp"
    #include "libdnf/transaction/Transformer.hpp"
    #include "libdnf/transaction/Types.hpp"
    using namespace libdnf;
%}


// This include has to be before %template definitions. SWIG is fragile :(
%include "libdnf/transaction/TransactionItemReason.hpp"
%include "libdnf/transaction/Types.hpp"


%template() std::vector<std::shared_ptr<libdnf::Transaction> >;
%template() std::vector<std::shared_ptr<libdnf::TransactionItem> >;
%template() std::vector<std::shared_ptr<libdnf::TransactionItemBase> >;
%template() std::vector<std::shared_ptr<libdnf::CompsGroupPackage> >;
%template() std::vector<std::shared_ptr<libdnf::CompsEnvironmentGroup> >;
%template(TransactionStateVector) std::vector<libdnf::TransactionState>;

%template() std::vector<uint32_t>;
%template() std::vector<int64_t>;
%template() std::vector<bool>;

%template() std::vector< std::string >;
%template() std::pair<int,std::string>;
%template() std::map<std::string,int>;
%template() std::map<std::string,std::string>;
%template() std::vector<std::pair<int,std::string> >;


// make SWIG look into following headers
%include "libdnf/transaction/Item.hpp"
%include "libdnf/transaction/CompsEnvironmentItem.hpp"
%include "libdnf/transaction/CompsGroupItem.hpp"
%include "libdnf/transaction/RPMItem.hpp"
%include "libdnf/transaction/Swdb.hpp"
%include "libdnf/transaction/Transaction.hpp"
%include "libdnf/transaction/TransactionItem.hpp"
%include "libdnf/transaction/MergedTransaction.hpp"
%include "libdnf/transaction/Transformer.hpp"
