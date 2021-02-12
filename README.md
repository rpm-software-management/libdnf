libdnf
======

This library provides a high level package-manager. It's core library of [dnf](https://github.com/rpm-software-management/dnf), [PackageKit](https://github.com/hughsie/PackageKit) and [rpm-ostree](https://github.com/projectatomic/rpm-ostree). It's replacement for deprecated [hawkey library](https://github.com/rpm-software-management/hawkey) which it contains inside and uses [librepo](https://github.com/rpm-software-management/librepo) under the hood.

:warning: :warning: :warning:
**Note that libdnf is currently being reworked and is
considered unstable. Once major users like PackageKit and
DNF are fully ported, a new stable release will be
considered.**
:warning: :warning: :warning:

License
----

LGPLv2+

Building for Fedora
===================

To install build requirements, run following command:

    dnf install check-devel cmake cppunit-devel gcc gcc-c++ glib2-devel gpgme-devel gtk-doc json-c-devel libmodulemd-devel librepo-devel libsolv-devel libsolv-tools make python2-devel python3-devel python2-sphinx python3-sphinx python2-breathe python3-breathe rpm-devel sqlite-devel swig libsmartcols-devel

From the checkout dir:

    mkdir build
    cd build/
    cmake .. -DPYTHON_DESIRED=3
    make

Building the documentation, from the build/ directory::

    make doc

Building RPMs:

    tito build --rpm --test

Tests
=====

All unit tests should pass after the build finishes:

    cd build
    make test

There are two parts of unit tests: unit tests in C and unit tests in Python. To run the C part of the tests manually, from hawkey checkout::

    build/tests/test_main tests/repos/

To manually execute the Python tests, from libdnf git checkout directory::

    PYTHONPATH=`readlink -f ./build/src/python/` python3 -m unittest discover -bt python/hawkey/tests/ -s python/hawkey/tests/tests/

The PYTHONPATH is unfortunately needed as the Python test suite needs to know where to import the built hawkey modules.

Contribution
============

Here's the most direct way to get your work merged into the project.

1. Fork the project
1. Clone down your fork
1. Implement your feature or bug fix and commit changes
1. If the change fixes a bug at [Red Hat bugzilla](https://bugzilla.redhat.com/), or if it is important to the end user, add the following block to the commit message:
    
       = changelog =
       msg:           message to be included in the changelog
       type:          one of: bugfix/enhancement/security (this field is required when message is present)
       resolves:      URLs to bugs or issues resolved by this commit (can be specified multiple times)
       related:       URLs to any related bugs or issues (can be specified multiple times)

   * For example::

         = changelog =
         msg: Do not close the database if it wasn't opened
         type: bugfix
         resolves: https://bugzilla.redhat.com/show_bug.cgi?id=1761976

   * For your convenience, you can also use git commit template by running the following command in the top-level directory of this project:

         git config commit.template ./.git-commit-template

1. In a separate commit, add your name and email under ``Libdnf CONTRIBUTORS`` section in the [authors file](https://github.com/rpm-software-management/libdnf/blob/dnf-4-master/README.md) as a reward for your generosity
1. Push the branch to your fork
1. Send a pull request for your branch

Please do not create pull requests with translation (.po) file improvements. Fix the translation on `Fedora Weblate <https://translate.fedoraproject.org/projects/dnf/>`_ instead.

Documentation
=============

See the [hawkey documentation page](http://hawkey.readthedocs.org).

Information internal to the hawkey development is maintained on a [github wiki](https://github.com/rpm-software-management/dnf/wiki#wiki-Contact).

Useful links
============

Bug database:

 * [libdnf github issues](https://github.com/rpm-software-management/libdnf/issues)
 * [bugzilla](https://bugzilla.redhat.com/buglist.cgi?bug_status=NEW&bug_status=ASSIGNED&bug_status=POST&bug_status=MODIFIED&bug_status=ON_DEV&bug_status=ON_QA&bug_status=VERIFIED&bug_status=RELEASE_PENDING&bug_status=CLOSED&component=libdnf&list_id=8513553&product=Fedora&query_format=advanced)
 
