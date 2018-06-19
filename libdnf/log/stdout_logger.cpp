#include <log4cxx/consoleappender.h>
#include <log4cxx/patternlayout.h>

#include "log.hpp"
#include "log_private.hpp"

void StdoutLogger::initialize(LogLevel log_level)
{
    log4cxx::PatternLayoutPtr layout = new log4cxx::PatternLayout("%m\n");
    log4cxx::helpers::Pool pool;

    log4cxx::ConsoleAppenderPtr appender(new log4cxx::ConsoleAppender(layout, log4cxx::ConsoleAppender::getSystemOut()));
    appender->addFilter(pImpl->filter_ptr);

    pImpl->logger_ptr->removeAllAppenders();
    pImpl->logger_ptr->addAppender(appender);

    set_log_level(log_level);
}

void StdoutLogger::set_log_level(LogLevel log_level)
{
    // set log level filter based on program verbosity
    switch (log_level) {
        case LogLevel::OFF:
            pImpl->filter_ptr->setLevelMin(log4cxx::Level::getOff());
            break;
        case LogLevel::QUIET:
            pImpl->filter_ptr->setLevelMin(log4cxx::Level::getInfo());
            break;
        case LogLevel::NORMAL:
            pImpl->filter_ptr->setLevelMin(log4cxx::Level::getInfo());
            break;
        case LogLevel::DEBUG:
            pImpl->filter_ptr->setLevelMin(log4cxx::Level::getDebug());
            break;
        case LogLevel::TRACE:
            pImpl->filter_ptr->setLevelMin(log4cxx::Level::getTrace());
            break;
    }
}
