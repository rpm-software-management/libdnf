#pragma once


#include "Goal.hpp"
#include "Package.hpp"
#include "Query.hpp"
#include "Sack.hpp"

#include "../base/base.hpp"



namespace libdnf::rpm {


/// class Base
///
class Base {
public:
    Base(libdnf::Base & dnfBase);

    /// @replaces dnf:dnf/base.py:attribute:Base.sack
    /// @replaces libdnf:libdnf/dnf-context.h:function:dnf_context_get_sack(DnfContext * context)
    void getSack();

    /// @replaces libdnf:libdnf/dnf-context.h:function:dnf_context_get_goal(DnfContext * context)
    void getGoal();

private:
    const libdnf::Base & dnfBase;
};


Base::Base(libdnf::Base & dnfBase)
    : dnfBase{dnfBase}
{
}


}  // namespace libdnf::rpm
