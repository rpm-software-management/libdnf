#pragma once


namespace libdnf {
class Base;
}  // namespace libdnf


#include <memory>
#include <string>

#include "Demands.hpp"
#include "Logger.hpp"

#include "../rpm/rpm.hpp"
#include "../comps/comps.hpp"


namespace libdnf {


/// Instances of :class:`dnf.Base` are the central point of functionality supplied by DNF. An application will typically create a single instance of this class which it will keep for the runt
/// ime needed to accomplish its packaging tasks. Plugins are managed by DNF and get a reference to :class:`dnf.Base` object when they run.
/// :class:`.Base` instances are stateful objects holding references to various data sources and data sinks. To properly finalize and close off any handles the object may hold, client code
/// should either call :meth:`.Base.close` when it has finished operations with the instance, or use the instance as a context manager. After the object has left the context, or its :meth:`.Base.
/// close` has been called explicitly, it must not be used. :meth:`.Base.close` will delete all downloaded packages upon successful transaction.
///
/// @replaces dnf:dnf/base.py:class:Base
class Base {
public:
    Base();

    /// instance of a :class:`libdnf::rpm::Base` class that encapsulates all rpm data and operations.
    std::unique_ptr<libdnf::rpm::Base> rpm;

    /// instance of a :class:`libdnf::comps::Base` class that encapsulates all comps data and operations.
    /// @replaces dnf:dnf/base.py:attribute:Base.comps
    std::unique_ptr<libdnf::comps::Base> comps;

    std::unique_ptr<Logger> logger;

    /*
    /// instance of a :class:`libdnf::module::Base` class that encapsulates all comps data and operations.
    std::unique_ptr<libdnf::module::Base> module;
    */

    // ACTIONS

    /// @replaces dnf:dnf/base.py:method:Base.autoremove(self, forms=None, pkg_specs=None, grp_specs=None, filenames=None)
    void autoRemove(const Demands & demands, std::vector<std::string> patterns, std::vector<std::string> excludePatterns);

    /// @replaces dnf:dnf/base.py:method:Base.distro_sync(self, pkg_spec=None)
    void distroSync(const Demands & demands, std::vector<std::string> patterns, std::vector<std::string> excludePatterns);

    /// @replaces dnf:dnf/base.py:method:Base.downgrade(self, pkg_spec)
    /// @replaces dnf:dnf/base.py:method:Base.downgrade_to(self, pkg_spec, strict=False)
    void downgrade(const Demands & demands, std::vector<std::string> patterns, std::vector<std::string> excludePatterns);

    /// @replaces dnf:dnf/base.py:method:Base.install(self, pkg_spec, reponame=None, strict=True, forms=None)
    /// @replaces dnf:dnf/base.py:method:Base.install_specs(self, install, exclude=None, reponame=None, strict=True, forms=None)
    void install(const Demands & demands, std::vector<std::string> patterns, std::vector<std::string> excludePatterns);

    /// @replaces dnf:dnf/base.py:method:Base.reinstall(self, pkg_spec, old_reponame=None, new_reponame=None, new_reponame_neq=None, remove_na=False)
    // TODO: specify additional args in demands? e.g. limit the operation to old/new repo etc.?
    void reinstall(const Demands & demands, std::vector<std::string> patterns, std::vector<std::string> excludePatterns);

    /// @replaces dnf:dnf/base.py:method:Base.remove(self, pkg_spec, reponame=None, forms=None)
    void remove(const Demands & demands, std::vector<std::string> patterns, std::vector<std::string> excludePatterns);

    /// @replaces dnf:dnf/base.py:method:Base.upgrade(self, pkg_spec, reponame=None)
    void upgrade(const Demands & demands, std::vector<std::string> patterns, std::vector<std::string> excludePatterns);

    /// @replaces dnf:dnf/base.py:method:Base.upgrade_all(self, reponame=None)
    void upgradeAll(const Demands & demands);
};


Base::Base()
    : rpm(std::make_unique<libdnf::rpm::Base>(*this))
    , comps(std::make_unique<libdnf::comps::Base>(*this))
    , logger(std::make_unique<Logger>(*this))
{
}


}  // namespace libdnf
