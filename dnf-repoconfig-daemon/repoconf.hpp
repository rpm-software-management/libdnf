#ifndef DNFDAEMON_REPOCONF_HPP
#define DNFDAEMON_REPOCONF_HPP

#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>

const std::string REPO_CONF_ERROR = "org.rpm.dnf.v1.rpm.RepoConf.Error";

using KeyValueMap = std::map<std::string, sdbus::Variant>;
using KeyValueMapList = std::vector<KeyValueMap>;

class RepoConf {
public:
    RepoConf(sdbus::IConnection &connection);
    ~RepoConf() {
        dbus_object->unregister();
    }
    void list(sdbus::MethodCall call);
    void get(sdbus::MethodCall call);
    void enable_disable(sdbus::MethodCall call, const bool enable);

private:
    KeyValueMapList repo_list(const std::vector<std::string> &ids);
    std::vector<std::string> enable_disable_repos(const std::vector<std::string> &ids, const bool enable);
    void dbus_register_methods();
    bool check_authorization(const std::string &actionid, const std::string &sender);
    sdbus::IConnection &connection;
    std::unique_ptr<sdbus::IObject> dbus_object;
};

#endif
