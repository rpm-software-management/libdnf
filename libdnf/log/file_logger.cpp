#include <log4cxx/consoleappender.h>
#include <log4cxx/rollingfileappender.h>
#include <log4cxx/patternlayout.h>

#include "log.hpp"
#include "log_private.hpp"

void FileLogger::initialize(LogLevel log_level, const std::string file_name, const std::string & file_size, int max_backup_index)
{
    log4cxx::PatternLayoutPtr layout = new log4cxx::PatternLayout("%d{ISO8601}{GMT}Z [%-5p] %m\n");
    log4cxx::helpers::Pool pool;

    log4cxx::AppenderPtr appender;
    if (file_name == "") {
        // file_name not specified
        // -> disable file logging by using a console appender with disabled logging
        log4cxx::ConsoleAppenderPtr appender_console(new log4cxx::ConsoleAppender());
        appender_console->setTarget(log4cxx::ConsoleAppender::getSystemErr());
        appender = appender_console;
    }
    else {
        bool append = true;
        log4cxx::RollingFileAppenderPtr appender_file(new log4cxx::RollingFileAppender(layout, file_name, append));
        appender_file->setMaxFileSize("1MB");
        appender_file->setMaxBackupIndex(3);
        appender = appender_file;
    }

    appender->addFilter(pImpl->filter_ptr);

    pImpl->logger_ptr->removeAllAppenders();
    pImpl->logger_ptr->addAppender(appender);

    set_log_level(log_level);
}

void FileLogger::set_log_level(LogLevel log_level)
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
            pImpl->filter_ptr->setLevelMin(log4cxx::Level::getDebug());
            break;
    }
}
