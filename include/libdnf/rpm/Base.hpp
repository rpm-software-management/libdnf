#ifndef LIBDNF_RPM_BASE_HPP
#define LIBDNF_RPM_BASE_HPP

// forward declarations
namespace libdnf::rpm {
class Base;
}  // namespace libdnf::rpm


#include "Goal.hpp"
#include "Package.hpp"
#include "Query.hpp"
#include "Sack.hpp"

#include "../base/Base.hpp"



namespace libdnf::rpm {


/// class Base
///
class Base {
public:
    Base(libdnf::Base & dnf_base);

    /// @replaces dnf:dnf/base.py:attribute:Base.sack
    /// @replaces libdnf:libdnf/dnf-context.h:function:dnf_context_get_sack(DnfContext * context)
    Sack get_sack() const;

    /// @replaces libdnf:libdnf/dnf-context.h:function:dnf_context_get_goal(DnfContext * context)
    void get_goal();

protected:
    const libdnf::Base & dnf_base;
};


Base::Base(libdnf::Base & dnf_base)
    : dnf_base{dnf_base}
{
}


}  // namespace libdnf::rpm

#endif
