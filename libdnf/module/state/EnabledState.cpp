#include "EnabledState.hpp"

void EnabledState::enable()
{
    throw EnabledException("Already enabled");
}

bool EnabledState::isEnabled()
{
    return true;
}
