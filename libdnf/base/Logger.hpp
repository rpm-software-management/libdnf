#pragma once


// forward declarations
namespace libdnf {
class Logger;
}  // namespace libdnf


#include "Base.hpp"

#include <string>


namespace libdnf {


/// Logger is a logging proxy that forwards data via callbacks.
/// It also buffers early logging data before logging is fully initiated.
///
/// @replaces libdnf:libdnf/log.hpp:class:Log
class Logger {
public:
    void trace(std::string msg);
    void debug(std::string msg);
    void info(std::string msg);
    void warning(std::string msg);
    void error(std::string msg);
//protected:
//    friend Base;
    const Base & dnf_base;
    Logger(Base & dnf_base);
};


Logger::Logger(Base & dnf_base)
    : dnf_base{dnf_base}
{
}


}  // namespace libdnf
