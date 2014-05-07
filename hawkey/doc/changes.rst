*************
 API Changes
*************

.. contents::

This document describes the API changes the library users should be aware of
before upgrading to each respective version. It is our plan to have the amount
of changes requiring changing the client code go to a minimum after the library
hits the 1.0.0 version.

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

All instantiations of ``hawkey.Repo`` now must be given the name of the Repo. The
following will now fail::

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
get the old behavior, append ``make_cache_dir=True`` to the Sack's constructor
arguments, that is change the following::

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

For old function behaviour use new function ``hy_query_filter_latest_per_arch()``

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
dropped without any replacement. However advisory types and severities are
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

Analogus to :ref:`core changes <0_4_15_core-label>`.

Package: string attributes are represented by Unicode object
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Attributes ``baseurl``, ``location``, ``sourcerpm``, ``version``, ``release``, ``name``, ``arch``, ``description``, ``evr``, ``license``, ``packager``, ``reponame``, ``summary`` and ``url`` of Package object return Unicode string.
