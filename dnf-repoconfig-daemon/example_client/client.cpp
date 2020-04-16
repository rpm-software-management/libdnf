#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>

int main(int argc, char *argv[])
{
    auto connection = sdbus::createSystemBusConnection();
    std::cout << "Client name: " << connection->getUniqueName() << std::endl;
    const char* destinationName = "org.rpm.dnf.v1.rpm.RepoConf";
    const char* objectPath = "/org/rpm/dnf/v1/rpm/RepoConf";
    auto repoconf_proxy = sdbus::createProxy(*connection, destinationName, objectPath);
    const char* interfaceName = "org.rpm.dnf.v1.rpm.RepoConf";
    repoconf_proxy->finishRegistration();

    {
        std::vector<std::string> ids = {"fedora", "updates"};
        std::vector<std::map<std::string, sdbus::Variant>> repolist;
        repoconf_proxy->callMethod("list").onInterface(interfaceName).withArguments(ids).storeResultsTo(repolist);
        for (auto &repo: repolist) {
            for (auto &item: repo) {
                std::cout << item.first << ": " << item.second.get<std::string>() << std::endl;
            }
            std::cout << "-----------" << std::endl;
        }
    }

    return 0;
}
