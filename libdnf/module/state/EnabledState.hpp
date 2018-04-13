#ifndef LIBDNF_ENABLEDSTATE_HPP
#define LIBDNF_ENABLEDSTATE_HPP


#include <stdexcept>
#include "ModuleState.hpp"

class EnabledState : public ModuleState
{
public:
    struct EnabledException : public std::runtime_error
    {
        explicit EnabledException(const char *what) : std::runtime_error(what) {}
        explicit EnabledException(const std::string &what) : std::runtime_error(what) {}
    };

    void enable() override;
    bool isEnabled() override;
};


#endif //LIBDNF_ENABLEDSTATE_HPP
