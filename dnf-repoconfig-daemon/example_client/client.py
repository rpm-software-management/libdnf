#!/usr/bin/env python3

import dbus

bus = dbus.SystemBus()
proxy = bus.get_object('org.rpm.dnf.v1.rpm.RepoConf', '/org/rpm/dnf/v1/rpm/RepoConf')
iface = dbus.Interface(proxy, dbus_interface='org.rpm.dnf.v1.rpm.RepoConf')

for repo in iface.list([]):
    print(repo['repoid'])


print(iface.get("fedora"))

#print(iface.get("fedoras"))

print(iface.disable(["fedora"]))
print(iface.enable(["fedora"]))
