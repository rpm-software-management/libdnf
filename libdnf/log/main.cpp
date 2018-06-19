#include "log.hpp"


// to compile, run:
// g++ main.cpp log.cpp file_logger.cpp stderr_logger.cpp stdout_logger.cpp --std=c++11 -I /usr/include/log4cxx/ -llog4cxx


int main()
{
    FileLogger file_logger("libdnf");
    file_logger.initialize(LogLevel::NORMAL, "foo.log", "10M", 1);

    // stderr logger with libdnf logger as a parent
    StderrLogger stderr_logger("libdnf.stderr");
    stderr_logger.initialize(LogLevel::DEBUG);

    // stdout logger with libdnf logger as a parent
    StdoutLogger stdout_logger("libdnf.stdout");
    stdout_logger.initialize(LogLevel::NORMAL);

    file_logger.info("(info) -> file logger only");

    stderr_logger.warn("(warn) -> stderr logger and file logger through inheritance");
    stderr_logger.debug("(debug) -> stderr logger and file logger through inheritance; nothing's displayed in log file due to log level");

    stdout_logger.info("(info) -> stdout logger and file logger through inheritance");
}
