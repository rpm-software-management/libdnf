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

%callback("%s_cb") ObjectQuery::get_string;
%callback("%s_cb") ObjectQuery::get_cstring;
%callback("%s_cb") ObjectQuery::get_boolean;
%callback("%s_cb") ObjectQuery::get_int32;
%callback("%s_cb") ObjectQuery::get_int64;
%callback("%s_cb") ObjectQuery::get_related_object;

%include "test/ObjectQuery.hpp"
%include "test/RelatedObject.hpp"
%template (RelatedObjectSetX) libdnf::utils::sack::Set<RelatedObjectWeakPtr>;
%template (ObjectRelatedQueryX) libdnf::utils::sack::Query<RelatedObjectWeakPtr>;
%include "test/RelatedObjectQuery.hpp"
%template (RelatedObjectSackX) libdnf::utils::sack::Sack<RelatedObject, RelatedObjectQuery>;
%include "test/RelatedObjectSack.hpp"
%template (ObjectSackX) libdnf::utils::sack::Sack<Object, ObjectQuery>;
%include "test/ObjectSack.hpp"
