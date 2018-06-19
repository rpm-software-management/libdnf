#include <log4cxx/consoleappender.h>
#include <log4cxx/filter/levelrangefilter.h>
#include <log4cxx/logger.h>
#include <log4cxx/logmanager.h>
#include <log4cxx/patternlayout.h>
#include <log4cxx/rollingfileappender.h>
#include <log4cxx/simplelayout.h>

#include "log.hpp"
#include "log_private.hpp"

Logger::Impl::Impl(const std::string & name)
{
    logger_ptr = log4cxx::Logger::getLogger(name);
    logger_ptr->setLevel(log4cxx::Level::getAll());
    filter_ptr = new log4cxx::filter::LevelRangeFilter();
}

Logger::Logger(const std::string & name)
:pImpl{new Impl(name)}
{
}

Logger::~Logger() = default;

void
Logger::fatal(const std::string & msg)
{
    pImpl->logger_ptr->fatal(msg);
}

void
Logger::error(const std::string & msg)
{
    pImpl->logger_ptr->error(msg);
}

void
Logger::warn(const std::string & msg)
{
    pImpl->logger_ptr->warn(msg);
}

void
Logger::info(const std::string & msg)
{
    pImpl->logger_ptr->info(msg);
}

void
Logger::debug(const std::string & msg)
{
    pImpl->logger_ptr->debug(msg);
}

void
Logger::trace(const std::string & msg)
{
    pImpl->logger_ptr->trace(msg);
}
