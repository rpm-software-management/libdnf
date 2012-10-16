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
