This is hawkey, library providing simplified C and Python API to libsolv
[1].

## Building for Fedora

Packages needed for the build (i.e. build requires):
* check-devel
* expat-devel
* libsolv-devel
* libsolv-tools
* rpm-devel
* zlib-devel

From your checkout dir:

    mkdir build
    cd build/
    cmake ..
    make

## Building with a custom version of libsolv

Libsolv is checked out at /home/akozumpl/libsolv, built at
/home/akozumpl/libsolv/build):

    mkdir build
    cd build/
    cmake -D LIBSOLV_PATH="/home/akozumpl/libsolv/" ..
    make

To trigger a rebuild of the libsolv header files remove solv/ in libsolv's build
directory.

## Unit tests

All unit tests should pass after the built finishes, from hawkey checkout dir:

    cd build/tests
    make tests

There are two parts of unit tests: unit tests in C and unit tests in
Python. Generally these two shouldn't overlap: the Python part should only test
Python-specific code (and perhaps functions that are too verbose to test from
C). To run the C part of the tests manually, from hawkey checkout):

    build/tests/test_main tests/repos/

Manually executing the Python part:

    PYTHONPATH=`readlink -f ./build/src/python/` nosetests -s tests/python/tests/

The PYTHONPATH is unfortunately needed as the Python test suite needs to know
where to import the built hawkey modules.

## More information

Collaborators are welcome, contact Ales Kozumplik <akozumpl@redhat.com> with
ideas and patches.

[1] https://github.com/openSUSE/libsolv
