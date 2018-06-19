#include <memory>

enum class LogLevel : int
{
    OFF = 0,    // file: off,   stderr: off,   stdout: off
    QUIET = 1,  // file: info,  stderr: warn,  stdout: info
    NORMAL = 2, // file: info,  stderr: info,  stdout: info
    DEBUG = 3,  // file: debug, stderr: debug, stdout: debug
    TRACE = 4,  // file: debug, stderr: trace, stdout: trace
};

class Logger {
public:
    Logger(const std::string & name);
    ~Logger();
    virtual void set_log_level(LogLevel log_level) = 0;
    void fatal(const std::string & msg);
    void error(const std::string & msg);
    void warn(const std::string & msg);
    void info(const std::string & msg);
    void debug(const std::string & msg);
    void trace(const std::string & msg);

protected:
    class Impl;
    const std::unique_ptr<Impl> pImpl;
};

class FileLogger : public Logger {
public:
    FileLogger(const std::string & name) : Logger(name) {};
    void initialize(LogLevel log_level, const std::string file_name, const std::string & file_size, int max_backup_index);
    void set_log_level(LogLevel log_level);
};

class StderrLogger : public Logger {
public:
    StderrLogger(const std::string & name) : Logger(name) {};
    void initialize(LogLevel log_level);
    void set_log_level(LogLevel log_level);
};

class StdoutLogger : public Logger {
public:
    StdoutLogger(const std::string & name) : Logger(name) {};
    void initialize(LogLevel log_level);
    void set_log_level(LogLevel log_level);
};
