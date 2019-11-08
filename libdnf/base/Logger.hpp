#pragma once


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
//private:
//    friend Base;
    const Base & dnfBase;
    Logger(Base & dnfBase);
};


Logger::Logger(Base & dnfBase)
    : dnfBase{dnfBase}
{
}


}  // namespace libdnf
