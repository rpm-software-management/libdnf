libdnf
======

Libdnf is a package management library.
It was originally written to support the [DNF](https://github.com/rpm-software-management/dnf/)
package manager and grew up into a versatile library.
Now you can use it for building custom tools that load repositories,
query packages, resolve dependencies and install packages.

It is powered with [libsolv](https://github.com/openSUSE/libsolv/), wrapping it with an easy to use programming interface.

* Libdnf supports working with the following artifacts:

  * RPM repositories (repomd)
  * RPM packages
  * Comps groups
  * Comps environments
  * Advisories (updateinfo, errata)
  * Modules (modulemd)

Libdnf interfaces with several programming languages with the following support:

 * C++ - fully supported
 * Python 3 - fully supported
 * Perl 5 - best effort
 * Ruby - best effort
 * Go - doesn't work, looking for contributors
 * C - not implemented, doesn't seem to be a priority for any of our existing API users


:warning: **The current (dnf-5-devel) branch is subject of a major rewrite. The API/ABI is currently unstable** :warning:

DNF5 Description (dnf-5-devel)
==============================

DNF5 Features
-------------

 * Replace Microdnf and DNF by a new tool that will be lightweight as Microdnf and fully featured as DNF
   * Decrease of a maintenance cost in the long term
 * Microdnf was developed as a simple packaging tool for containers without the requirement of Python. Nowadays it is
   also used for other use cases. We are getting requests for additional features (CLI options, commands,
   configuration), to eliminate a gap between DNF and Microdnf
 * Fully integrated Modularity in LIBDNF workflows
   * Modularity is supported in DNF and LIBDNF but it is not fully integrated. Integration was not possible due to
     limitation of compatibility with other tools (PackageKit)
     * Fully integrated modularity requires changes in library workflow
 * Unified user interface
   * DNF/YUM was developed for decades with impact of multiple styles and naming conventions (options, configuration
     options, commands)
 * Plugins
   * DNF plugins are not applicable for PackageKit and Microdnf (e.g. versionlock, subscription-manager), therefore
     PackageKit behaves differently to DNF
   * New plugins (C++, Python) will be available for all users
     * unified behaviour
     * Removal of functional duplicities (subscription-manager functionality)
       * Decrease maintenance cost
 * Shared configurations
   * In DNF4 the configuration is only partially honored by PackageKit and Microdnf
 * New Daemon
   * The new daemon can provide an alternative to PackageKit for RPMs (only one backend of PackageKit) if it will be
     integrated into Desktop
 * Additional improvements
   * Reports in structure (API)
     * DNF reports a lot of important information only in logs
 * Shared cache and improved cache handling (Optional, not for RHEL10 GA)
   * Microdnf, DNF4, and PackageKit use cached repositories on a different location with different cache structure.
 * Performance improvement
   * Loading of repositories
   * Advisory operation
   * RPM query
     * Name filters with a case-insensitive search
   * Smart sharing of metadata between dnf, microdnf, daemon
     * Reduce disk and downloads requirements

Major codebase improvements
---------------------------

 * Removal of duplicated implementation
   * LIBDNF evolved from LIBHIF (PackageKit library) and HAWKEY (DNF library). The integration was never finished.
     LIBDNF still contains duplicated functionality.
   * decrease of the code maintenance cost in future
 * Unify python bindings
   * Libdnf provides two types of Python bindings
     * CPython (hawkey)
     * SWIG (libdnf)
   * Maintaining and communication between both bindings requires a lot of resources
   * Unifying bindings is not possible without breaking compatibility
 * SWIG bindings
   * With SWIG we can generate additional bindings without spending huge resources
   * Code in particular languages will be very similar to each other
 * Separation of system state from history DB and `/etc/dnf/module.d`
   * In dnf-4 the list of userinstalled packages and list of installed groups along with the lists of packages installed
     from them is computed as an aggregation of transaction history. In dnf5 it will be stored separately, having
     multiple benefits, among them that the history database will serve for informational purposes only and will not
     define the state of the system (it gets corrupted occasionally etc.).
   * Data stored in `/etc/dnf/module.d` were not supposed to be user modifiable and their format is not sufficient
     (missing information about installed packages with installed profiles)

Documentation
=============

* For HTML documentation see https://libdnf.readthedocs.io/
* The header files are documented because documentation is mainly generated from them


Reporting issues
================

* [Red Hat Bugzilla](https://bugzilla.redhat.com/enter_bug.cgi?product=Fedora&component=libdnf) is the preferred way of filing issues. [[backlog](https://bugzilla.redhat.com/buglist.cgi?bug_status=__open__&product=Fedora&component=libdnf)]
* [GitHub issues](https://github.com/rpm-software-management/libdnf/issues/new) are also accepted. [[backlog](https://github.com/rpm-software-management/libdnf/issues)]


Contributing
============

* By contributing to this project you agree to the Developer Certificate of Origin (DCO).
  This document is a simple statement that you, as a contributor,
  have the legal right to submit the contribution. See the [DCO](DCO) file for details.
* All contributions to this project are licensed under [LGPLv2.1+](lgpl-2.1.txt) or [GPLv2+](gpl-2.0.txt).
  See the [License](#license) section for details.


Writing patches
---------------

* Please follow the [contributing guidelines](https://libdnf.readthedocs.io/en/dnf-5-devel/contributing/index.html)
* When a patch is ready, submit a pull request
* It is a good practice to write documentation and unit tests as part of the patches


Building
--------
To install build requirements, run::

    $ dnf builddep libdnf.spec --define '_with_sanitizers 1' [--define '_without_<option> 1 ...]

To build code, run::

    $ mkdir build
    $ cd build
    $ cmake .. -DWITH_SANITIZERS=ON [-DWITH_<OPTION>=<ON|OFF> ...]
    $ make -j4

To build rpms from git, run::

    $ export PREFIX=$(rpmspec libdnf.spec -q --srpm --qf '%{name}-%{version}'); git archive --format=tar.gz --prefix=$PREFIX/ HEAD > $PREFIX.tar.gz
    $ rpmbuild -ba --define "_sourcedir $(pwd)" libdnf.spec [--with=<option>|--without=<option> ...]


Testing
-------
To run the tests, follow the steps to build the code and then run::

    # from the 'build' directory
    $ CTEST_OUTPUT_ON_FAILURE=1 make test

As an alternative, tests can be executed in a verbose mode::

    # from the 'build' directory
    $ make test ARGS='-V'


Translating
-----------
TBD


License
=======

* The libraries is licensed under [LGPLv2.1+](lgpl-2.1.txt)
* The standalone programs that are part of this project are licensed under [GPLv2+](gpl-2.0.txt)
* See [COPYING](COPYING.md) for more details
