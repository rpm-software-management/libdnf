%module comps

%include <exception.i>
%include <std_string.i>

#if defined(SWIGPYTHON)
%import(module="libdnf.common") "common.i"
#elif defined(SWIGRUBY)
%import(module="libdnf/common") "common.i"
#elif defined(SWIGPERL)
%include "std_vector.i"
%import(module="libdnf::common") "common.i"
#endif

%{
    #include "libdnf/comps/comps.hpp"
    #include "libdnf/comps/group/group.hpp"
    #include "libdnf/comps/group/query.hpp"
    #include "libdnf/comps/group/sack.hpp"
    using namespace libdnf::comps;
%}

#define CV __perl_CV

%template(GroupWeakPtr) libdnf::WeakPtr<Group, false>;
%template(GroupWeakPtrSet) libdnf::Set<libdnf::comps::GroupWeakPtr>;
%template(GroupWeakPtrQuery) libdnf::sack::Query<libdnf::comps::GroupWeakPtr>;

%include "libdnf/comps/comps.hpp"
%include "libdnf/comps/group/group.hpp"
%include "libdnf/comps/group/query.hpp"
%include "libdnf/comps/group/sack.hpp"
