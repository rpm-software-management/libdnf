libdnf
======

This library provides a high level package-manager. It uses librepo and hawkey
under the hood.

:warning: :warning: :warning:
**Note that libhif is currently being reworked and is
considered unstable. Once major users like PackageKit and
DNF are fully ported, a new stable release will be
considered.**
:warning: :warning: :warning:

License
----

LGPLv2+

Building for Fedora
===================

Packages needed for the build, or the build requires:

* make
* check-devel
* cmake
* gcc
* libsolv-devel
* libsolv-tools
* librepo-devel
* python-devel (or python3-devel for Python 3 build)
* python-nose (or python3-nose for Python 3 build)
* rpm-devel
* glib2-devel
* gtk-doc
* gobject-introspection-devel
* python-sphinx

From the checkout dir::

    mkdir build
    cd build/
    cmake .. # add '-DPYTHON_DESIRED="3"' option for Python 3 build
    make

Building the documentation, from the build/ directory::

    make doc

Building RPMs:

    tito init # Step is required due to removal of tito support from the project
    tito build --rpm --test

Tests
=====

All unit tests should pass after the build finishes:

    cd build/tests
    make tests

There are two parts of unit tests: unit tests in C and unit tests in Python. To run the C part of the tests manually, from hawkey checkout::

    build/tests/test_main tests/repos/

Manually executing the Python tests::

    PYTHONPATH=`readlink -f ./build/src/python/` nosetests -s tests/python/tests/

The PYTHONPATH is unfortunately needed as the Python test suite needs to know where to import the built hawkey modules.

Documentation
=============

See the `hawkey documentation page <http://hawkey.readthedocs.org>`_.

Information internal to the hawkey development is maintained on a `github wiki <https://github.com/rpm-software-management/dnf/wiki#wiki-Contact>`_.
