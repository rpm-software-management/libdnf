libdnf plugin interface
=======================

Motivation
----------
[DNF](https://github.com/rpm-software-management/dnf) package manager that uses the libdnf library, has plugins support. There are useful DNF plugins and some of them needs to be run during each operation. Of course, DNF plugins are run only if user use DNF. If the user will use another application ([microdnf](https://github.com/rpm-software-management/microdnf), [PackageKit](https://github.com/hughsie/PackageKit), ...) DNF plugins are skipped. This is effort to move plugins into lower level - into libdnf. So the plugins will be running even if another application is used.

The libdnf plugins interface is under development now. We cannot simply copy plugin interface from DNF. DNF is a Pytnon application but libdnf is a C/C++ library. That also means not all DNF plugins can be transferred to libdnf. Moreover there is more ways how application can use the libdnf library. DNF use another way (implements upper logic in python) than microdnf and PackageKit (which use libdnf "context" code). It takes time to unify it. Hooks are inserted in "context" code of libdnf now.

Status of prototype
-------------------
* Plugins are .so libraries.
* Plugins are readed from directory (/usr/lib64/libdnf/plugins is hardcoded now).
* Plugins are loaded in alphabetical order (sorted by filename).
* Initializations and hooks are called in the same order as plugins were loaded.
* Freeing of plugins is in the reverse order as they were loaded.
* Plugins have access to libdnf context and can use libdnf public functions to read and modify it.
* each plugin must implement 4 functions:
```
        const PluginInfo * pluginGetInfo(void);
        PluginHandle * pluginInitHandle(int version, PluginMode mode, void * initData);
        void pluginFreeHandle(PluginHandle * handle);
        int pluginHook(PluginHandle * handle, PluginHookId id, void * hookData, PluginHookError * error);
```

Plugin functions
----------------
* `const PluginInfo * pluginGetInfo(void)`
> Returns information about the plugin.

* `PluginHandle * pluginInitHandle(int version, PluginMode mode, void * initData)`
> Initialization of new plugin instance. Returns handle to it.

* `void pluginFreeHandle(PluginHandle * handle)`
> Frees plugin instance.

* `int pluginHook(PluginHandle * handle, PluginHookId id, void * hookData, PluginHookError * error)`
> Called by hook. Each hook is identified by PluginHookId.

Ideas
-----
* Only links to plugins .so files will be in the directory.
* Plugin can be enabled/disabled by adding/removing the link to it. Link name will start with number which defines order of plugin loading. Eg. there are plugins pluginA, pluginB, pluginC, pluginD and we want to use 3 of them in specific order. So we make appropriate links 05pluginA, 10pluginD, 15pluginB.