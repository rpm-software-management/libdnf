#include <log4cxx/filter/levelrangefilter.h>
#include <log4cxx/logger.h>


class Logger::Impl {
public:
    Impl(const std::string & name);
    log4cxx::LoggerPtr logger_ptr;
    log4cxx::filter::LevelRangeFilterPtr filter_ptr;
};
