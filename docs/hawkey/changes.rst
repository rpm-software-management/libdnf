..
  Copyright (C) 2014-2015  Red Hat, Inc.

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

*************
 API Changes
*************

.. contents::

Introduction
============

This document describes the API changes the library users should be aware of before upgrading to each respective version. It is our plan to have the amount of changes requiring changing the client code go to a minimum after the library hits the 1.0.0 version.

Depracated API items (classes, methods, etc.) are designated as such in this document. The first release where support for such items can be dropped entirely must be issued at *least five months* after the issue of the release that announced the deprecation and at the same time have, relatively to the deprecating release, either:

* a higher major version number, or
* a higher minor version number, or
* a patchlevel number that is *by at least five* greater.

These criteria are likely to tighten in the future as hawkey matures.

Actual changes in the API are then announced in this document as well. ABI changes including changes in functions' parameter counts or types or removal of public symbols from ``libhawkey`` imply an increase in the library's SONAME version.


Changes in 0.2.10
=================

Python bindings
---------------

:meth:`Query.filter` now returns a new instance of :class:`Query`, the same as
the original with the new filtering applied. This allows for greater flexibility
handling the :class:`Query` objects and resembles the way ``QuerySets`` behave in
Django.

In practice the following code will stop working as expected::

  q = hawkey.Query(self.sack)
  q.filter(name__eq="flying")
  # processing the query ...

It needs to be changed to::

  q = hawkey.Query(self.sack)
  q = q.filter(name__eq="flying")
  # processing the query ...

The original semantics is now available via the :meth:`Query.filterm` method, so
the following will also work::

  q = hawkey.Query(self.sack)
  q.filterm(name__eq="flying")
  # processing the query ...

Changes in 0.2.11
=================

Python bindings
---------------

In Python's :class:`Package` instances accessors for string attributes now
return None instead of the empty string if the attribute is missing (for instance
a ``pkg.sourcerpm`` now returns None if ``pkg`` is a source rpm package
already).

This change is towards a more conventional Python practice. Also, this leaves the
empty string return value free to be used when it is actually the case.

Changes in 0.3.0
================

Core
----

Query: key for reponame filtering
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The Query key value used for filtering by the repo name is ``HY_PKG_REPONAME``
now (was ``HY_PKG_REPO``). The old value was misleading.

Repo initialization
^^^^^^^^^^^^^^^^^^^

``hy_repo_create()`` for Repo object initialization now needs to be passed a
name of the repository.

.. _changes_query_installs:

Query installs obsoleted
^^^^^^^^^^^^^^^^^^^^^^^^

All Goal methods accepting Query as the means of selecting packages, such as
``hy_goal_install_query()`` have been replaced with their Selector
counterparts. Selector structures have been introduced for the particular
purpose of specifying a package that best matches the given criteria and at the
same time is suitable for installation. For a discussion of this decision see
:ref:`rationale_selectors`.


Python bindings
---------------

Query: filtering by repository with the reponame key
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Similar change happened in Python, the following constructs::

  q = q.filter(repo="updates")

need to be changed to::

  q = q.filter(reponame="updates")

The old version of this didn't allow using the same string to both construct the
query and dynamically get the reponame attribute from the returned packages
(used e.g. in DNF to search by user-specified criteria).

Package: removed methods for direct EVR comparison
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following will no longer work::

  if pkg.evr_eq(some_other_pkg):
      ...

Instead use the result of ``pkg.evr_cmp``, for instance::

  if pkg.evr_cmp(some_other_pkg) == 0:
      ...

This function compares only the EVR part of a package, not the name. Since it
rarely make sense to compare versions of packages of different names, the
following is suggested::

  if pkg == some_other_pkg:
      ...

Repo initialization
^^^^^^^^^^^^^^^^^^^

All instantiations of :class:`hawkey.Repo` now must be given the name of the
Repo. The following will now fail::

  r = hawkey.Repo()
  r.name = "fedora"

Use this instead::

  r = hawkey.Repo("fedora")

Query installs obsoleted
^^^^^^^^^^^^^^^^^^^^^^^^

See :ref:`changes_query_installs` in the C section. In Python Queries will no
longer work as goal target specifiers, the following will fail::

  q = hawkey.Query(sack)
  q.filter(name="gimp")
  goal.install(query=q)

Instead use::

  sltr = hawkey.Selector(sack)
  sltr.set(name="gimp")
  goal.install(select=sltr)

Or a convenience notation::

  goal.install(name="gimp")

Changes in 0.3.1
================

Query: ``hy_query_filter_package_in()`` takes a new parameter
-------------------------------------------------------------

``keyname`` parameter was added to the function signature. The new parameter
allows filtering by a specific relation to the resulting packages, for
instance::

  hy_query_filter_package_in(q, HY_PKG_OBSOLETES, HY_EQ, pset)

only leaves the packages obsoleting a package in ``pset`` a part of the result.

Removed ``hy_query_filter_obsoleting()``
----------------------------------------

The new version of ``hy_query_filter_package_in()`` handles this now, see above.

In Python, the following is no longer supported::

  q = query.filter(obsoleting=1)

The equivalent new syntax is::

  installed = hawkey.Query(sack).filter(reponame=SYSTEM_REPO_NAME)
  q = query.filter(obsoletes=installed)

Changes in 0.3.2
================

Removed ``hy_packagelist_of_obsoletes``.
----------------------------------------

The function was not systematic. Same result is achieved by obtaining obsoleting
reldeps from a package and then trying to find the installed packages that
provide it. In Python::

  q = hawkey.Query(sack).filter(reponame=SYSTEM_REPO_NAME, provides=pkg.obsoletes)

Changes in 0.3.3
================

Renamed ``hy_package_get_nvra`` to ``hy_package_get_nevra``
-----------------------------------------------------------

The old name was by error, the functionality has not changed: this function has
always returned the full NEVRA, skipping the epoch part when it's 0.

Changes in 0.3.4
================

Python bindings
---------------

``pkg.__repr__()`` is more verbose now
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Previously, ``repr(pkg)`` would yield for instance ``<_hawkey.Package object,
id: 5>``. Now more complete information is present, including the package's
NEVRA and repository: ``<hawkey.Package object id 5, foo-2-9\.noarch,
@System>``.

Also notice that the representation now mentions the final ``hawkey.Package``
type, not ``_hawkey.Package``. Note that these are currently the same.

Changes in 0.3.8
================

Core
----

New parameter ``rootdir`` to ``hy_sack_create()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``hy_sack_create()`` now accepts third argument, ``rootdir``. This can be used
to tell Hawkey that we are intending to do transactions in a changeroot, not in
the current root. It effectively makes use of the RPM database found under
``rootdir``. To make your code compile in 0.3.8 without changing functionality, change::

    HySack sack = hy_sack_create(cachedir, arch);

to::

    HySack sack = hy_sack_create(cachedir, arch, NULL);

Python bindings
---------------

Forms recognized by ``Subject`` are no longer an instance-scope setting
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It became necessary to differentiate between the default forms used by
``subject.nevra_possibilities()`` and
``subject.nevra_possibilities_real()``. Therefore there is little sense in
setting the default form for an entire ``Subject`` instance. The following
code::

  subj = hawkey.Subject("input", form=hawkey.FORM_NEVRA)
  result = list(subj.nevra_possibilities())

is thus replaced by::

  subj = hawkey.Subject("input")
  result = list(subj.nevra_possibilities(form=hawkey.FORM_NEVRA))

Changes in 0.3.9
================

Core
----

Flags for ``hy_sack_create``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``hy_sack_create()`` now accepts fourth argument, ``flags``, introduced to
modify the sack behavior with boolean flags. Currently only one flag is
supported, ``HY_MAKE_CACHE_DIR``, which causes the cache directory to be created
if it doesn't exist yet. To preserve the previous behavior, change the
following::

    HySack sack = hy_sack_create(cachedir, arch, rootdir);

into::

    HySack sack = hy_sack_create(cachedir, arch, rootdir, HY_MAKE_CACHE_DIR);

``hy_sack_get_cache_path`` is renamed to ``hy_sack_get_cache_dir``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Update your code by mechanically replacing the name.


Python bindings
---------------

``make_cache_dir`` for Sack's constructor
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A new sack by default no longer automatically creates the cache directory. To
get the old behavior, append ``make_cache_dir=True`` to the
:meth:`.Sack.__init__` arguments, that is change the following::

    sack = hawkey.Sack(...)

to::

    sack = hawkey.Sack(..., make_cache_dir=True)


``cache_path`` property of ``Sack`` renamed to ``cache_dir``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Reflects the similar change in C API.

Changes in 0.3.11
=================

.. _0_3_11_core-label:

Core
----

``hy_goal_package_obsoletes()`` removed, ``hy_goal_list_obsoleted_by_package()`` provided instead
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``hy_goal_package_obsoletes()`` was flawed in that it only returned a single
obsoleted package (in general, package can obsolete arbitrary number of packages
and upgrade a package of the same name which is also reported as an
obsolete). Use ``hy_goal_list_obsoleted_by_package()`` instead, to see the
complete set of packages that inclusion of the given package in an RPM
transaction will cause to be removed.

``hy_goal_list_erasures()`` does not report obsoletes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In other words, ``hy_goal_list_erasures()`` and ``hy_goal_list_obsoleted()``
return disjoint sets.


Python bindings
---------------

Directly reflecting the :ref:`core changes <0_3_11_core-label>`. In particular,
instead of::

    obsoleted_pkg = goal.package_obsoletes(pkg)

use::

    obsoleted = goal.obsoleted_by_package(pkg) # list
    obsoleted_pkg = obsoleted[0]

Changes in 0.4.5
=================

Core
----

Query: ``hy_query_filter_latest()`` now filter latest packages ignoring architecture
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For old function behavior use new function ``hy_query_filter_latest_per_arch()``

Python bindings
---------------

In Python's :class:`Query` option ``latest`` in :meth:`Query.filter` now filter
only the latest packages ignoring architecture. The original semantics for filtering
latest packages for each arch is now available via ``latest_per_arch`` option.

For example there are these packages in sack::

  glibc-2.17-4.fc19.x86_64
  glibc-2.16-24.fc18.x86_64
  glibc-2.16-24.fc18.i686

  >>> q = hawkey.Query(self.sack).filter(name="glibc")
  >>> map(str, q.filter(latest=True))
  ['glibc-2.17-4.fc19.x86_64']

  >>> map(str, q.filter(latest_per_arch=True))
  ['glibc-2.17-4.fc19.x86_64', 'glibc-2.16-24.fc18.i686']

Changes in 0.4.13
=================

Core
----

Deprecated ``hy_package_get_update_*``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The functions were deprecated because there can be multiple advisories referring
to a single package. Please use the new function ``hy_package_get_advisories()``
which returns all these advisories. New functions ``hy_advisory_get_*`` provide
the data retrieved by the deprecated functions.

The only exception is the ``hy_package_get_update_severity()`` which will be
dropped without any replacement. However advisory types and severity levels are
distinguished from now and the type is accessible via ``hy_advisory_get_type()``.
Thus enum ``HyUpdateSeverity`` was also deprecated. A new ``HyAdvisoryType``
should be used instead.

The old functions will be dropped after 2014-07-07.

Changes in 0.4.15
=================

.. _0_4_15_core-label:

Core
----

``hy_goal_write_debugdata()`` takes a directory parameter
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``hy_goal_write_debugdata()`` has a new `const char *dir` argument to communicate the target directory for the debugging data. The old call::

    hy_goal_write_debugdata(goal);

should be changed to achieve the same behavior to::

    hy_goal_write_debugdata(goal, "./debugdata");

Python bindings
---------------

``Goal.write_debugdata()`` takes a directory parameter
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Analogous to :ref:`core changes <0_4_15_core-label>`.

Package: string attributes are represented by Unicode object
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Attributes ``baseurl``, ``location``, ``sourcerpm``, ``version``, ``release``, ``name``, ``arch``, ``description``, ``evr``, ``license``, ``packager``, ``reponame``, ``summary`` and ``url`` of Package object return Unicode string.


Changes in 0.4.18
=================

Core
----

Deprecated ``hy_advisory_get_filenames``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The function was deprecated because we need more information about packages
listed in an advisory than just file names. Please use the new function
``hy_advisory_get_packages()`` in combination with
``hy_advisorypkg_get_string()`` to obtain the data originally provided by the
deprecated function.

The old function will be dropped after 2014-10-15 AND no sooner than in 0.4.21.

Python bindings
---------------

``Repo()`` does not accept ``cost`` keyword argument
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Instead of::

  r = hawkey.Repo('name', cost=30)

use::

  r = hawkey.Repo('name')
  r.cost = 30

Also previously when no ``cost`` was given it defaulted to 1000. Now the default is 0. Both these aspects were present by mistake and the new interface is consistent with the C library.

Deprecated ``_hawkey.Advisory.filenames``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The attribute was deprecated because the underlying C function was also
deprecated. Please use the new attribute ``packages`` and the attribute
``filename`` of the returned objects to obtain the data originally provided by
the deprecated attribute.

The old attribute will be dropped after 2014-10-15 AND no sooner than in 0.4.21.


Changes in 0.4.19
=================

Python bindings
---------------

Advisory attributes in Unicode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

All string attributes of ``Advisory`` and ``AdvisoryRef`` objects (except the
deprecated ``filenames`` attribute) are Unicode objects now.


Changes in 0.5.2
=================

Core
----

``hy_chksum_str`` returns NULL
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Previously, the function ``hy_chksum_str`` would cause a segmentation fault when it was used
with incorrect type value. Now it correctly returns NULL if type parameter does not correspond
to any of expected values.


Changes in 0.5.3
================

Core
----

New parameter ``logfile`` to ``hy_sack_create()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``hy_sack_create()`` now accepts fifth argument, ``logfile`` to customize log file path.
If NULL parameter as ``logfile`` is given, then all debug records are written to ``hawkey.log``
in ``cachedir``. To make your code compile in 0.5.3 without changing functionality, change::

    HySack sack = hy_sack_create(cachedir, arch, rootdir, 0);

to::

    HySack sack = hy_sack_create(cachedir, arch, rootdir, NULL, 0);

Deprecated ``hy_create_cmdline_repo()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The function will be removed since `hy_add_cmdline_package` creates cmdline repository automatically.

The function will be dropped after 2015-06-23 AND no sooner than in 0.5.8.

Python bindings
---------------

New optional parameter ``logfile`` to ``Sack`` constructor
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This addition lets user specify log file path from :meth:`.Sack.__init__`

``cache_path`` property of ``Sack`` renamed to ``cache_dir``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This change was already announced but it actually never happened.

Deprecated ``Sack`` method ``create_cmdline_repo()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The method will be removed since :meth:`.Sack.add_cmdline_package` creates cmdline repository automatically.

The method will be dropped after 2015-06-23 AND no sooner than in 0.5.8.


Changes in 0.5.4
================

Python bindings
---------------

Goal: ``install()`` takes a new optional parameter
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If the ``optional`` parameter is set to ``True``, hawkey silently skips packages that can not
be installed.


Changes in 0.5.5
================

Core
----

Renamed ``hy_sack_load_yum_repo`` to ``hy_sack_load_repo``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Hawkey is package manager agnostic and the ``yum`` phrase could be misleading.

The function will be dropped after 2015-10-27 AND no sooner than in 0.5.8.

Python bindings
---------------

Sack method `load_yum_repo` has been renamed to :meth:`.Sack.load_repo`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Hawkey is package manager agnostic and the ``yum`` phrase could be misleading.

The method will be dropped after 2015-10-27 AND no sooner than in 0.5.8.


Changes in 0.5.7
================

Python bindings
---------------

Package: file attribute is represented by list of Unicode objects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Sack: `list_arches` method returns list of Unicode objects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


Changes in 0.5.9
================

Core
----

Deprecated ``hy_goal_req_has_distupgrade()``, ``hy_goal_req_has_erase()`` and ``hy_goal_req_has_upgrade()`` functions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To make your code compile in 0.5.9 without changing functionality, change::

    hy_goal_req_has_distupgrade_all(goal)
    hy_goal_req_has_erase(goal)
    hy_goal_req_has_upgrade_all(goal)

to::

    hy_goal_has_actions(goal, HY_DISTUPGRADE_ALL)
    hy_goal_has_actions(goal, HY_ERASE)
    hy_goal_has_actions(goal, HY_UPGRADE_ALL)

respectively


Python bindings
---------------

Deprecated Goal methods :meth:`Goal.req_has_distupgrade_all`, :meth:`Goal.req_has_erase` and :meth:`Goal.req_has_upgrade_all`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To make your code compatible with hawkey 0.5.9 without changing functionality,
change::

    goal.req_has_distupgrade_all()
    goal.req_has_erase()
    goal.req_has_upgrade_all()

to::

    goal.actions & hawkey.DISTUPGRADE_ALL
    goal.actions & hawkey.ERASE
    goal.actions & hawkey.UPGRADE_ALL

respectively


Changes in 0.6.2
================

Core
----

The ``hy_advisory_get_filenames()`` API call, the corresponding Python
property ``filenames`` of class :class:`Advisory` are removed.
Instead, iterate over ``hy_advisory_get_packages()`` with
``hy_advisorypkg_get_string()`` and ``HY_ADVISORYPKG_FILENAME``.  No
known hawkey API consumers were using this call.

Hawkey now has a dependency on GLib.  Aside from the above
``hy_advisory_get_filenames()`` call, the Python API is fully
preserved.  The C API has minor changes, but the goal is to avoid
causing a significant amount of porting work for existing consumers.

The ``hy_package_get_files`` API call now returns a ``char **``,
allocated via `g_malloc`.  Free with `g_strfreev`.

The ``HyStringArray`` type is removed, as nothing now uses it.

:class:`HyPackageList` is now just a :class:`GPtrArray`, though
the existing API is converted into wrappers.  Notably, this means
you can now use ``g_ptr_array_unref()``.


Python bindings
---------------

Aside from the one change below, the Python bindings should be
unaffected by the C API changes.

Advisory: The ``filename`` property is removed along with the C API
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
