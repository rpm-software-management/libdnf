
#include "repoconf.hpp"

#include <sdbus-c++/sdbus-c++.h>
#include <string>


int main(int argc, char *argv[])
{
    // Create D-Bus connection to the system bus and requests name on it.
    const char* serviceName = "org.rpm.dnf.v1.rpm.RepoConf";
    auto connection = sdbus::createSystemBusConnection(serviceName);

    auto repo_conf = RepoConf(*connection);

    // Run the I/O event loop on the bus connection.
    connection->enterEventLoop();
}
