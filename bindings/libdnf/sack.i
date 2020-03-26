%module sack

%include <stdint.i>
%include <std_string.i>
%include <std_vector.i>

%{
    #include "libdnf/utils/sack/set.hpp"
    #include "libdnf/utils/sack/sack.hpp"
    #include "libdnf/utils/sack/query.hpp"
    #include "libdnf/utils/sack/query_cmp.hpp"
    #include "libdnf/utils/weak_ptr.hpp"

    #include "test/object.hpp"
    #include "test/object_query.hpp"
    #include "test/object_sack.hpp"
    #include "test/related_object.hpp"
    #include "test/related_object_query.hpp"
    #include "test/related_object_sack.hpp"
%}

// SWIG does not support "final" C++ keyword, we ignore all "final" in code
#define final

%include "libdnf/utils/sack/set.hpp"
%include "libdnf/utils/sack/sack.hpp"
%include "libdnf/utils/sack/query.hpp"
%include "libdnf/utils/sack/query_cmp.hpp"
%include "libdnf/utils/weak_ptr.hpp"

%include "test/object.hpp"
%template (ObjectVector) std::vector<Object>;
%template (ObjectSetX) libdnf::utils::sack::Set<ObjectWeakPtr>;
%template (ObjectQueryX) libdnf::utils::sack::Query<ObjectWeakPtr>;

%callback("%s_cb") ObjectQuery::get_string;
%callback("%s_cb") ObjectQuery::get_cstring;
%callback("%s_cb") ObjectQuery::get_boolean;
%callback("%s_cb") ObjectQuery::get_int32;
%callback("%s_cb") ObjectQuery::get_int64;
%callback("%s_cb") ObjectQuery::get_related_object;
%callback("%s_cb") ObjectQuery::match_installed;
%callback("%s_cb") ObjectQuery::match_repoid;

%include "test/object_query.hpp"
%include "test/related_object.hpp"
%template (RelatedObjectSetX) libdnf::utils::sack::Set<RelatedObjectWeakPtr>;
%template (ObjectRelatedQueryX) libdnf::utils::sack::Query<RelatedObjectWeakPtr>;
%include "test/related_object_query.hpp"
%template (RelatedObjectSackX) libdnf::utils::sack::Sack<RelatedObject, RelatedObjectQuery>;
%include "test/related_object_sack.hpp"
%template (ObjectSackX) libdnf::utils::sack::Sack<Object, ObjectQuery>;
%include "test/object_sack.hpp"
