
#include "logger.hpp"
#include "repoconf.hpp"

#include "../libdnf/log.hpp"
#include "../libdnf/utils/tinyformat/tinyformat.hpp"

#include <sdbus-c++/sdbus-c++.h>
#include <string>


int main(int argc, char *argv[])
{
    JournalLogger journal_logger;
    libdnf::Log::setLogger(&journal_logger);

    // Create D-Bus connection to the system bus and requests name on it.
    const char* serviceName = "org.rpm.dnf.v1.rpm.RepoConf";
    std::unique_ptr<sdbus::IConnection> connection = NULL;
    try {
        connection = sdbus::createSystemBusConnection(serviceName);
    } catch (const sdbus::Error &e) {
        journal_logger.error(tfm::format("Fatal error: %s", e.what()));
        return 1;
    }

    auto repo_conf = RepoConf(*connection);

    // Run the I/O event loop on the bus connection.
    connection->enterEventLoop();
}
