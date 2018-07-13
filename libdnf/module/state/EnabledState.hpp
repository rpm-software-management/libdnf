#ifndef LIBDNF_ENABLEDSTATE_HPP
#define LIBDNF_ENABLEDSTATE_HPP


#include <stdexcept>
#include "ModuleState.hpp"

class EnabledState : public ModuleState
{
public:
    bool isEnabled() override;
};


#endif //LIBDNF_ENABLEDSTATE_HPP
