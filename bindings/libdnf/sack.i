%module sack

%include <stdint.i>
%include <std_string.i>
%include <std_vector.i>

%{
    #include "libdnf/utils/WeakPtr.hpp"
    #include "libdnf/utils/sack/Set.hpp"
    #include "libdnf/utils/sack/Sack.hpp"
    #include "libdnf/utils/sack/QueryCmp.hpp"
    #include "libdnf/utils/sack/Query.hpp"

    #include "test/Object.hpp"
    #include "test/ObjectQuery.hpp"
    #include "test/ObjectSack.hpp"
    #include "test/RelatedObject.hpp"
    #include "test/RelatedObjectQuery.hpp"
    #include "test/RelatedObjectSack.hpp"
%}

// SWIG does not support "final" C++ keyword, we ignore all "final" in code
#define final

%include "libdnf/utils/WeakPtr.hpp"
%include "libdnf/utils/sack/Set.hpp"
%include "libdnf/utils/sack/Sack.hpp"
%include "libdnf/utils/sack/QueryCmp.hpp"
%include "libdnf/utils/sack/Query.hpp"

%include "test/Object.hpp"
%template (ObjectVector) std::vector<Object>;
%template (ObjectSetX) libdnf::utils::sack::Set<ObjectWeakPtr>;
%template (ObjectQueryX) libdnf::utils::sack::Query<ObjectWeakPtr>;

// SWIG has problem with callback functions in classes and namespaces :(
// so ignore they (using regexp and full path in name)
%rename ("$ignore", regextarget=1, fullname=1) "ObjectQuery::get_.*";
// and add symbols into global namespace
%{
auto ObjectQuery_get_string = ObjectQuery::get_string;
auto ObjectQuery_get_cstring = ObjectQuery::get_cstring;
auto ObjectQuery_get_boolean = ObjectQuery::get_boolean;
auto ObjectQuery_get_int32 = ObjectQuery::get_int32;
auto ObjectQuery_get_int64 = ObjectQuery::get_int64;
auto ObjectQuery_get_related_object = ObjectQuery::get_related_object;
%}

%callback("%s_cb");
std::string ObjectQuery_get_string(const ObjectWeakPtr & obj);
std::string ObjectQuery_get_cstring(const ObjectWeakPtr & obj);
bool ObjectQuery_get_boolean(const ObjectWeakPtr & obj);
int64_t ObjectQuery_get_int32(const ObjectWeakPtr & obj);
int64_t ObjectQuery_get_int64(const ObjectWeakPtr & obj);
std::vector<std::string> ObjectQuery_get_related_object(const ObjectWeakPtr & obj);
%nocallback;

%include "test/ObjectQuery.hpp"
%include "test/RelatedObject.hpp"
%template (RelatedObjectSetX) libdnf::utils::sack::Set<RelatedObjectWeakPtr>;
%template (ObjectRelatedQueryX) libdnf::utils::sack::Query<RelatedObjectWeakPtr>;
%include "test/RelatedObjectQuery.hpp"
%template (RelatedObjectSackX) libdnf::utils::sack::Sack<RelatedObject, RelatedObjectQuery>;
%include "test/RelatedObjectSack.hpp"
%template (ObjectSackX) libdnf::utils::sack::Sack<Object, ObjectQuery>;
%include "test/ObjectSack.hpp"
