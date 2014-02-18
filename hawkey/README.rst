========
 Hawkey
========

This is hawkey, library providing simplified C and Python API to `libsolv <https://github.com/openSUSE/libsolv>`_.

Building for Fedora
===================

Packages needed for the build, or the build requires:

* check-devel
* cmake
* expat-devel
* gcc
* libsolv-devel
* libsolv-tools
* python-devel (or python3-devel for Python 3 build)
* python-nose (or python3-nose for Python 3 build)
* rpm-devel
* zlib-devel

From the checkout dir::

    mkdir build
    cd build/
    cmake .. # add '-DPYTHON_DESIRED="3"' option for Python 3 build
    make

Building the documentation, from the build/ directory::

    make doc

Building from a libsolv checkout
================================

Libsolv is checked out at ``/home/akozumpl/libsolv``, build dir is at ``/home/akozumpl/libsolv/build``::

    mkdir build
    cd build/
    cmake -D LIBSOLV_PATH="/home/akozumpl/libsolv/" ..
    make

Tests
=====

All unit tests should pass after the build finishes, from the hawkey checkout::

    cd build/tests
    make tests

There are two parts of unit tests: unit tests in C and unit tests in Python. To run the C part of the tests manually, from hawkey checkout::

    build/tests/test_main tests/repos/

Manually executing the Python tests::

    PYTHONPATH=`readlink -f ./build/src/python/` nosetests -s tests/python/tests/

The PYTHONPATH is unfortunately needed as the Python test suite needs to know where to import the built hawkey modules.

Documentation
=============

See the `hawkey documentation page <http://akozumpl.github.io/hawkey/>`_.

Information internal to the hawkey development is maintained on a `github wiki <https://github.com/akozumpl/dnf/wiki#wiki-Contact>`_.
