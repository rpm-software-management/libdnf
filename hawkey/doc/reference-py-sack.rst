..
  Copyright (C) 2015  Red Hat, Inc.

  This copyrighted material is made available to anyone wishing to use,
  modify, copy, or redistribute it subject to the terms and conditions of
  the GNU General Public License v.2, or (at your option) any later version.
  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY expressed or implied, including the implied warranties of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
  Public License for more details.  You should have received a copy of the
  GNU General Public License along with this program; if not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.  Any Red Hat trademarks that are incorporated in the
  source code or documentation are not subject to the GNU General Public
  License and may only be used or replicated with the express permission of
  Red Hat, Inc.

*******************************************
Sack---The fundamental hawkey structure
*******************************************

.. class:: hawkey.Sack

  Instances of :class:`hawkey.Sack` represent collections of packages. An
  application typically needs at least one instance because it provides much of
  the hawkey's functionality.

  .. warning:: Any package instance is not supposed to work interchangeably between
               :class:`hawkey.Query`, :class:`hawkey.Selector` or :class:`hawkey.Goal`
               created from different :class:`hawkey.Sack`. Usually for common tasks
               there is no need to initialize two or more `Sacks` in your program.
               Sacks cannot be deeply copied.

  .. attribute:: cache_dir

    A read-only string property giving the path to the location where a
    metadata cache is stored.

  .. attribute:: installonly

    A write-only sequence of strings property setting the provide names of
    packages that should only ever be installed, never upgraded.

  .. attribute:: installonly_limit

    A write-only integer property setting how many installonly packages with
    the same name are allowed to be installed concurrently. If ``0``, any
    number of packages can be installed.

  .. method:: __init__(\
    cachedir=_CACHEDIR, arch=_ARCH, rootdir=_ROOTDIR, pkgcls=hawkey.Package, \
    pkginitval=None, make_cache_dir=False, logfile=_LOGFILE)

    Initialize the sack with a default cache directory, log file location set
    to ``hawkey.log`` in the cache directory, an automatically detected
    architecture and the current root (``/``) as an installroot. The cache is
    disabled by default.

    `cachedir` is a string giving a path of a cache location.

    `arch` is a string specifying an architecture.

    `rootdir` is a string giving a path to an installroot.

    `pkgcls` is a class of packages retrieved from the sack. The class'
    ``__init__`` method must accept two arguments. The first argument is a tuple
    of the sack and the ID of the package. The second argument is the
    `pkginitval` argument. `pkginitval` cannot be ``None`` if `pkgcls` is
    specified.

    `make_cache_dir` is a boolean that specifies whether the cache should be
    used to speedup loading of repositories or not (see
    :ref:`\building_and_reusing_the_repo_cache-label`).

    `logfile` is a string giving a path of a log file location.

  .. method:: __len__()

    Returns the number of the packages loaded into the sack.

  .. method:: add_cmdline_package(filename)

    Add a package to a command line repository and return it. The package is specified as a string `filename` of an RPM file. The command line repository will be automatically created if doesn't exist already. It could be referenced later by :const:`hawkey.CMDLINE_REPO_NAME` name.

  .. method:: add_excludes(packages)

    Add a sequence of packages that cannot be fetched by Queries nor Selectors.

  .. method:: add_includes(packages)

    Add a sequence of the only packages that can be fetched by Queries or
    Selectors.

    This is the inverse operation of :meth:`add_excludes`. Any package that
    is not in the union of all the included packages is excluded. This works in
    conjunction with exclude and doesn't override it. So, if you both include
    and exclude the same package, the package is considered excluded no matter
    of the order.

  .. method:: disable_repo(name)

    Disable the repository identified by a string *name*. Packages in that
    repository cannot be fetched by Queries nor Selectors.

  .. method:: enable_repo(name)

    Enable the repository identified by a string *name*. Packages in that
    repository can be fetched by Queries or Selectors.

  .. warning:: Execution of :meth:`add_excludes`, :meth:`add_includes`,
               :meth:`disable_repo` or :meth:`enable_repo` methods could cause
               inconsistent results in previously evaluated :class:`.Query`,
               :class:`.Selector` or :class:`.Goal`. The rule of thumb is
               to exclude/include packages, enable/disable repositories at first and
               then do actual computing using :class:`.Query`, :class:`.Selector`
               or :class:`.Goal`. For more details see 
               `developer discussion <https://github.com/rpm-software-management/hawkey/pull/87>`_.

  .. method:: evr_cmp(evr1, evr2)

    Compare two EVR strings and return a negative integer if *evr1* < *evr2*,
    zero if *evr1* == *evr2* or a positive integer if *evr1* > *evr2*.

  .. method:: get_running_kernel()

    Detect and return the package of the currently running kernel. If the
    package cannot be found, ``None`` is returned.

  .. method:: list_arches()

    List strings giving all the supported architectures.

  .. method:: load_system_repo(repo=None, build_cache=False)

    Load the information about the packages in the system repository (in Fedora
    it is the RPM database) into the sack. This makes the dependency solving
    aware of the already installed packages. The system repository is always
    set to :const:`hawkey.SYSTEM_REPO_NAME`. The information is not written to
    the cache by default.

    `repo` is an optional :class:`.Repo` object that represents the system
    repository. The object is updated during the loading.

    `build_cache` is a boolean that specifies whether the information should be
    written to the cache (see :ref:`\building_and_reusing_the_repo_cache-label`).

  .. method:: load_repo(\
    repo, build_cache=False, load_filelists=False, load_presto=False, \
    load_updateinfo=False)

    Load the information about the packages in a :class:`.Repo` into the sack.
    This makes the dependency solving aware of these packages. The information
    is not written to the cache by default.

    `repo` is the :class:`.Repo` object to be processed. At least its
    :attr:`.Repo.repomd_fn` must be set. If the cache has to be updated,
    :attr:`.Repo.primary_fn` is needed too. Some information about the loading
    process and some results of it are written into the internal state of the
    repository object.

    `build_cache` is a boolean that specifies whether the information should be
    written to the cache (see :ref:`\building_and_reusing_the_repo_cache-label`).

    `load_filelists`, `load_presto` and `load_updateinfo` are booleans that
    specify whether the :attr:`.Repo.filelists_fn`, :attr:`.Repo.presto_fn` and
    :attr:`.Repo.updateinfo_fn` files of the repository should be processed.
    These files may contain information needed for dependency solving,
    downloading or querying of some packages. Enable it if you are not sure (see
    :ref:`\case_for_loading_the_filelists-label`).
